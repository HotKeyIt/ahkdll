#include "pch.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"
#include <process.h>  // NewThread

SHORT returnCount = 0;
enum TTVArgType;
void TokenToVariant(ExprTokenType& aToken, VARIANT& aVar, TTVArgType* aVarIsArg = FALSE);

EXPORT unsigned int NewThread(LPCTSTR aScript, LPCTSTR aCmdLine, LPCTSTR aTitle) // HotKeyIt check if dll is ready to execute
{
	DWORD result = 0;
	SIZE_T aParamLen = _tcslen(aScript ? aScript : _T("Persistent")) + _tcslen(aCmdLine) + 1 + _tcslen(aTitle) + 1 + 3;
	LPTSTR aScriptCmdLine = tmalloc(aParamLen);
	memset(aScriptCmdLine, 0, aParamLen * sizeof(TCHAR));
	_tcscpy(aScriptCmdLine, aScript ? aScript : _T("Persistent"));
	_tcscpy(aScriptCmdLine + _tcslen(aScript ? aScript : _T("")) + 1, aCmdLine ? aCmdLine : _T(""));
	_tcscpy(aScriptCmdLine + _tcslen(aScript ? aScript : _T("")) + _tcslen(aCmdLine ? aCmdLine : _T("")) + 2, aTitle ? aTitle : _T(""));
	unsigned int ThreadID;
	TCHAR buf[MAX_INTEGER_LENGTH];
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void *))&ThreadMain, aScriptCmdLine, 0, &ThreadID);
	if (hThread)
	{
		CloseHandle(hThread);
		sntprintf(buf, _countof(buf), _T("ahk%d"), ThreadID);
		HANDLE hEvent = CreateEvent(NULL, false, false, buf);
		// we need to give the thread also little time to allocate memory to avoid std::bad_alloc that might happen when you run newthread in a loop
		if (WaitForSingleObject(hEvent, 1000) != WAIT_TIMEOUT)
			result = ThreadID;
		CloseHandle(hEvent);
	}
	return result;
}
// Following macros are used in addScript and ahkExec
// HotExpr code from LoadFromFile, Hotkeys need to be toggled to get activated
#define FINALIZE_HOTKEYS \
	if (Hotkey::sHotkeyCount > HotkeyCount)\
	{\
		Hotstring::SuspendAll(!g_IsSuspended);\
		Hotkey::ManifestAllHotkeysHotstringsHooks();\
		Hotstring::SuspendAll(g_IsSuspended);\
		Hotkey::ManifestAllHotkeysHotstringsHooks();\
	}
#define RESTORE_IF_EXPR \
	if (g_FirstHotExpr)\
	{\
		if (aLastHotExpr)\
			aLastHotExpr->NextExpr = g_FirstHotExpr;\
		if (aFirstHotExpr)\
			g_FirstHotExpr = aFirstHotExpr;\
	}\
	else\
	{\
		g_FirstHotExpr = aFirstHotExpr;\
		g_LastHotExpr = aLastHotExpr;\
	}
// AutoHotkey needs to be running at this point
#define BACKUP_G_SCRIPT \
	int aCurrFileIndex = g_script->mCurrFileIndex, aCombinedLineNumber = g_script->mCombinedLineNumber;\
	bool aNextLineIsFunctionBody = g_script->mNextLineIsFunctionBody;\
	Line *aFirstLine = g_script->mFirstLine,*aLastLine = g_script->mLastLine,*aCurrLine = g_script->mCurrLine;\
	g_script->mNextLineIsFunctionBody = false;\
	Func *aCurrFunc  = g->CurrentFunc;\
	int aClassObjectCount = g_script->mClassObjectCount;\
	g_script->mClassObjectCount = NULL;\
	g_script->mFirstLine = NULL;g_script->mLastLine = NULL;\
	g_script->mIsReadyToExecute = false;\
	g->CurrentFunc = NULL;

#define RESTORE_G_SCRIPT \
	g_script->mFirstLine = aFirstLine;\
	g_script->mLastLine = aLastLine;\
	g_script->mLastLine->mNextLine = NULL;\
	g_script->mCurrLine = aCurrLine;\
	g_script->mClassObjectCount = aClassObjectCount + g_script->mClassObjectCount;\
	g_script->mCurrFileIndex = aCurrFileIndex;\
	g_script->mNextLineIsFunctionBody = aNextLineIsFunctionBody;\
	g_script->mCombinedLineNumber = aCombinedLineNumber;
#ifndef _USRDLL
//COM virtual functions
int com_ahkPause(LPTSTR aChangeTo, DWORD aThreadID){return ahkPause(aChangeTo, aThreadID);}
UINT_PTR com_ahkFindLabel(LPTSTR aLabelName, DWORD aThreadID){return ahkFindLabel(aLabelName, aThreadID);}
LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar, DWORD aThreadID){return ahkgetvar(name,getVar, aThreadID);}
unsigned int com_ahkassign(LPTSTR name, LPTSTR value, DWORD aThreadID){return ahkassign(name,value, aThreadID);}
UINT_PTR com_ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait, DWORD aThreadID){return ahkExecuteLine(line,aMode,wait, aThreadID);}
int com_ahkLabel(LPTSTR aLabelName, unsigned int nowait, DWORD aThreadID){return ahkLabel(aLabelName,nowait, aThreadID);}
UINT_PTR com_ahkFindFunc(LPTSTR funcname, DWORD aThreadID){return ahkFindFunc(funcname, aThreadID);}
LPTSTR com_ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10, DWORD aThreadID){return ahkFunction(func,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10, aThreadID);}
UINT_PTR com_addScript(LPTSTR script, int waitexecute, DWORD aThreadID){return addScript(script,waitexecute, aThreadID);}
int com_ahkExec(LPTSTR script, DWORD aThreadID){return ahkExec(script, aThreadID);}
unsigned int com_newthread(LPTSTR script,LPTSTR argv, LPTSTR title){return NewThread(script,argv, title);}
int com_ahkReady(DWORD aThreadID){return ahkReady(aThreadID);}
#endif

EXPORT int ahkReady(DWORD aThreadID) // HotKeyIt check if dll is ready to execute
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	bool IsReadyToExecute = false;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					IsReadyToExecute = g_script->mIsReadyToExecute;
					curr_teb->ThreadLocalStoragePointer = tls;
					break;
				}
			}
			return IsReadyToExecute;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
			IsReadyToExecute = g_script->mIsReadyToExecute;
			curr_teb->ThreadLocalStoragePointer = tls;
		}
	}
#ifndef _USRDLL
	return IsReadyToExecute;
#else
	return IsReadyToExecute && !g_Reloading;
#endif
}

EXPORT int ahkPause(LPTSTR aChangeTo, DWORD aThreadID) //Change pause state of a running script
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}

	if (!g->IsPaused && ((size_t)aChangeTo == 1 || *aChangeTo == '1' || ((*aChangeTo == 'O' || *aChangeTo == 'o') && (*(aChangeTo + 1) == 'N' || *(aChangeTo + 1) == 'n'))))
	{
		Hotkey::ResetRunAgainAfterFinished();
		g->IsPaused = true;
		++g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
		g_script->UpdateTrayIcon();
	}
	else if (g->IsPaused)
	{
		g->IsPaused = false;
		--g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
		g_script->UpdateTrayIcon();
	}
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return (int)g->IsPaused;
}

EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname, DWORD aThreadID)
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	UINT_PTR result = (UINT_PTR)g_script->FindFunc(funcname);
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return result;
}

EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName, DWORD aThreadID)
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	UserFunc *aCurrentFunc = g->CurrentFunc;
	g->CurrentFunc = NULL;
	UINT_PTR result = (UINT_PTR)g_script->FindLabel(aLabelName);
	g->CurrentFunc = aCurrentFunc;
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return result;
}

// Naveen: v1. ahkgetvar()
EXPORT LPTSTR ahkgetvar(LPTSTR name, unsigned int getVar, DWORD aThreadID)
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	HANDLE ahThread = NULL;
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	BYTE aSlot = InterlockedIncrement16(&returnCount) & 0xFFF;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return _T("");
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	if (g_MainThreadID != ThreadID)
	{
		ahThread = OpenThread(THREAD_ALL_ACCESS, TRUE, ThreadID);
		SuspendThread(ahThread);
	}
	auto varlist = g_script->GlobalVars();
	int insert_pos;
	Var *ahkvar = varlist->Find(name, &insert_pos);
	if (getVar != NULL)
	{
		if (ahkvar->mType == VAR_VIRTUAL)
		{
			if (ahThread)
				ResumeThread(ahThread);

			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
			return _T("");
		}
		LPTSTR new_mem = (LPTSTR)realloc((LPTSTR )g_result_to_return_dll[aSlot],MAX_INTEGER_LENGTH);
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (ahThread)
				ResumeThread(ahThread);
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
			return _T("");
		}
		g_result_to_return_dll[aSlot] = new_mem;
		if (ahThread)
			ResumeThread(ahThread);
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return ITOA64((UINT_PTR)ahkvar, g_result_to_return_dll[aSlot]);
	}
	else if ( !ahkvar->HasContents() )
	{
		if (ahThread)
			ResumeThread(ahThread);
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return _T("");
	}
	LPTSTR aContents = ahkvar->Contents(true);
	if (!(*aContents))
	{
		LPTSTR new_mem = (LPTSTR )realloc((LPTSTR )g_result_to_return_dll[aSlot],(MAX_NUMBER_LENGTH + sizeof(TCHAR)));
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (ahThread)
				ResumeThread(ahThread);
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
			return _T("");
		}
		g_result_to_return_dll[aSlot] = new_mem;
		if (_tcsicmp(name, _T("A_IsPaused"))) //ahkvar->mVV == BIV_IsPaused)
		{
			++g; // imitate new thread for A_IsPaused
			ITOA64(ahkvar->ToInt64(), new_mem);
			--g;
		}
		else
			ITOA64(ahkvar->ToInt64(), new_mem);
	}
	else
	{
		LPTSTR new_mem = (LPTSTR)realloc((LPTSTR)g_result_to_return_dll[aSlot], ahkvar->ByteLength() + sizeof(TCHAR));
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (ahThread)
				ResumeThread(ahThread);
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
			return _T("");
		}
		g_result_to_return_dll[aSlot] = new_mem;
		memcpy(new_mem, aContents, ahkvar->ByteLength() + sizeof(TCHAR));
	}
	if (ahThread)
		ResumeThread(ahThread);
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return g_result_to_return_dll[aSlot];
}	

EXPORT int ahkassign(LPTSTR name, LPTSTR value, DWORD aThreadID) // ahkwine 0.1
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	HANDLE ahThread = NULL;
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return -1;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return -1; // AutoHotkey needs to be running at this point //
	}
	if (g_MainThreadID != ThreadID)
	{
		ahThread = OpenThread(THREAD_ALL_ACCESS, TRUE, ThreadID);
		SuspendThread(ahThread);
	}
	auto varlist = g_script->GlobalVars();
	int insert_pos;
	Var *var = varlist->Find(name, &insert_pos);
	if (!(var = g_script->FindVar(name, 0, FINDVAR_GLOBAL)))
	{
		if (ahThread)
			ResumeThread(ahThread);
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return -1;  // Realistically should never happen.
	}
	var->Assign(value);
	if (ahThread)
		ResumeThread(ahThread);
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return 0; // success
}

//HotKeyIt ahkExecuteLine()
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line, unsigned int aMode, unsigned int wait, DWORD aThreadID)
{
	HWND msghWnd = g_hWnd;
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	Line *templine = (Line *)line;
	if (templine == NULL)
	{
		UINT_PTR result = (UINT_PTR)g_script->mFirstLine;
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return result;
	}
	msghWnd = g_hWnd;
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	if (aMode)
	{
		if (wait)
			SendMessage(msghWnd, AHK_EXECUTE, (WPARAM)templine, (LPARAM)aMode);
		else
			PostMessage(msghWnd, AHK_EXECUTE, (WPARAM)templine, (LPARAM)aMode);
	}
	if (templine = templine->mNextLine)
		if (templine->mActionType == ACT_BLOCK_BEGIN && templine->mAttribute)
		{
			for(;!(templine->mActionType == ACT_BLOCK_END && templine->mAttribute);)
				templine = templine->mNextLine;
			templine = templine->mNextLine;
		}
	return (UINT_PTR) templine;
}

EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait, DWORD aThreadID) // 0 = wait = default
{
	HWND msghWnd = g_hWnd;
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	Label *aLabel = g_script->FindLabel(aLabelName) ;
	if (aLabel)
	{
		msghWnd = g_hWnd;
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		if (nowait)
			PostMessage(msghWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		else
			SendMessage(msghWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		return 1;
	}
	else
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0;
	}
}

// HotKeyIt: addScript()
EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute, DWORD aThreadID)
{   // dynamically include a script from text!!
	// labels, hotkeys, functions.
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point // LOADING_FAILED cant be used due to PTR return type
	}
	int HotkeyCount = Hotkey::sHotkeyCount;
	HotkeyCriterion *aFirstHotExpr = g_FirstHotExpr,*aLastHotExpr = g_LastHotExpr;
	g_FirstHotExpr = NULL;g_LastHotExpr = NULL;
	//int a_guiCount = g_guiCount;
	//g_guiCount = 0;

	LPCTSTR aPathToShow = g_script->mCurrLine->mArg ? g_script->mCurrLine->mArg->text : g_script->mFileSpec;
	BACKUP_G_SCRIPT
	if (g_script->LoadFromText(script,aPathToShow) != OK) // || !g_script->PreparseBlocks(oldLastLine->mNextLine)))
	{
		g->CurrentFunc = (UserFunc *)aCurrFunc;
		RESTORE_G_SCRIPT
		//g_guiCount = a_guiCount;
		RESTORE_IF_EXPR
		g_script->mIsReadyToExecute = true;
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0;  // LOADING_FAILED cant be used due to PTR return type
	}
	FINALIZE_HOTKEYS
	RESTORE_IF_EXPR
	g_script->mIsReadyToExecute = true;
	g->CurrentFunc = (UserFunc *)aCurrFunc;
	if (waitexecute != 0)
	{
		if (waitexecute == 1)
		{
			g_ReturnNotExit = true;
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script->mFirstLine, (LPARAM)NULL);
			g_ReturnNotExit = false;
		}
		else
			PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script->mFirstLine, (LPARAM)NULL);
	}
	Line *aTempLine = g_script->mFirstLine;
	aLastLine->mNextLine = aTempLine;
	aTempLine->mPrevLine = aLastLine;
	aLastLine = g_script->mLastLine;
	RESTORE_G_SCRIPT
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
	return (UINT_PTR) aTempLine;
}

EXPORT int ahkExec(LPTSTR script, DWORD aThreadID)
{   // dynamically include a script from text!!
	// labels, hotkeys, functions
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					break;
				}
			}
			if (!curr_teb)
				return 0;
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
		}
	}
	if (!g_script || !g_script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point // LOADING_FAILED cant be used due to PTR return type.
	}
	int HotkeyCount = Hotkey::sHotkeyCount;
	HotkeyCriterion *aFirstHotExpr = g_FirstHotExpr,*aLastHotExpr = g_LastHotExpr;
	g_FirstHotExpr = NULL;g_LastHotExpr = NULL;
	BACKUP_G_SCRIPT
	int aFuncCount = g_script->mFuncs.mCount; 
	Func **aFunc = (Func**)malloc(g_script->mFuncs.mCount*sizeof(Func));
	for (int i = 0; i < g_script->mFuncs.mCount; i++)
	{
		aFunc[i] = g_script->mFuncs.mItem[i];
	}
	int aSourceFileIdx = Line::sSourceFileCount;

	// Backup SimpleHeap to restore later
	SimpleHeap *aSimpleHeap = new SimpleHeap();
	SimpleHeap *bkpSimpleHeap = g_SimpleHeap;
	g_SimpleHeap = aSimpleHeap;

	if ((g_script->LoadFromText(script) != OK)) // || !g_script->PreparseBlocks(oldLastLine->mNextLine))
	{
		g->CurrentFunc = (UserFunc *)aCurrFunc;
		// Delete used and restore SimpleHeap
		g_SimpleHeap = bkpSimpleHeap;
		aSimpleHeap->DeleteAll();
		delete aSimpleHeap;
		for (int i = 0; i < g_script->mFuncs.mCount; i++)
		{
			g_script->mFuncs.mItem[i] = aFuncCount < i ? NULL : aFunc[i];
		}
		free(aFunc);
		RESTORE_G_SCRIPT
		RESTORE_IF_EXPR
		g_script->mFuncs.mCount = aFuncCount;
		g_script->mIsReadyToExecute = true;
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
	return NULL;
	}
	FINALIZE_HOTKEYS
	RESTORE_IF_EXPR
	g_script->mIsReadyToExecute = true;
	g->CurrentFunc = (UserFunc *)aCurrFunc;
	Line *aTempLine = g_script->mLastLine;
	Line *aExecLine = g_script->mFirstLine;
	for (int i = 0; i < g_script->mFuncs.mCount; i++)
	{
		g_script->mFuncs.mItem[i] = aFuncCount<i ? NULL : aFunc[i];
	}
	free(aFunc);
	RESTORE_G_SCRIPT
	g_script->mFuncs.mCount = aFuncCount;
	g_ReturnNotExit = true;
	// Restore SimpleHeap so functions will use correct memory
	g_SimpleHeap = bkpSimpleHeap;
	SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)aExecLine, (LPARAM)NULL);
	
	g_ReturnNotExit = false;
	Line *prevLine = aTempLine->mPrevLine;
	for(; prevLine; prevLine = prevLine->mPrevLine)
	{
		prevLine->mNextLine->FreeDerefBufIfLarge();
		delete prevLine->mNextLine;
	}
	delete aExecLine;
	Line::sSourceFileCount = aSourceFileIdx;
	// Delete used and restore SimpleHeap
	aSimpleHeap->DeleteAll();
	delete aSimpleHeap;
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
	return OK;
}

#define INITFUNCANDTOKEN \
	if (aFuncAndToken.buf == NULL) { \
		if (!(aFuncAndToken.buf = (LPTSTR)malloc(MAX_INTEGER_SIZE * sizeof(TCHAR)))	|| !(aFuncAndToken.param = (ExprTokenType**)malloc(sizeof(ExprTokenType**) * 10))) \
			MemoryError(); \
		aFuncAndToken.mToken.buf = aFuncAndToken.buf; \
		for (int i = 0; i < 10; i++) \
		{ \
			aFuncAndToken.params[i].SetVar(new Var()); \
			aFuncAndToken.param[i] = &aFuncAndToken.params[i]; \
		} \
	}

EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10, DWORD aThreadID)
{
	HWND msghWnd = g_hWnd;
	Script *script = g_script;
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					msghWnd = g_hWnd;
					script = g_script;
					break;
				}
			}
			if (!curr_teb)
			{
				curr_teb->ThreadLocalStoragePointer = tls;
				return _T("");
			}
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
			msghWnd = g_hWnd;
			script = g_script;
		}
	}
	if (!g_script || !script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return 0; // AutoHotkey needs to be running at this point //
	}
	Func *aFunc = script->FindFunc(func) ;
	if (aFunc)
	{
		FuncAndToken &aFuncAndToken = g_FuncAndTokenToReturn[InterlockedIncrement16(&returnCount) & 0xFFF];
		INITFUNCANDTOKEN
		if (!aFuncAndToken.buf)
		{ // init aFuncAndToken

		}
		int aParamsCount = 0;
		LPTSTR *params[10] = {&param1,&param2,&param3,&param4,&param5,&param6,&param7,&param8,&param9,&param10};
		for (;aParamsCount < 10;aParamsCount++)
			if (!*params[aParamsCount])
				break;
		if (aParamsCount < aFunc->mMinParams)
		{
			script->ScriptError(ERR_TOO_FEW_PARAMS, func);
			return _T("");
		}
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
		for (int i = 0;(aFunc->mParamCount > i || aFunc->mIsVariadic) && aParamsCount>i;i++)
			aFuncAndToken.params[i].var->Assign(*params[i]);
		aFuncAndToken.mFunc = aFunc ;
		SendMessage(msghWnd, AHK_EXECUTE_FUNCTION, (WPARAM)&aFuncAndToken, NULL);
		return aFuncAndToken.result_to_return_dll;
	}
	else // Function not found
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return _T("");
	}
}




EXPORT int ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10, DWORD aThreadID)
{
	HWND msghWnd = g_hWnd;
	Script *script = g_script;
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_MainThreadID != ThreadID || (aThreadID && aThreadID != g_MainThreadID))
	{
		if (aThreadID)
		{
			for (int i = 0; i < MAX_AHK_THREADS; i++)
			{
				if (g_ahkThreads[i][5] == aThreadID)
				{
					curr_teb = (PMYTEB)NtCurrentTeb();
					tls = curr_teb->ThreadLocalStoragePointer;
					curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
					msghWnd = g_hWnd;
					script = g_script;
					break;
				}
			}
			if (!curr_teb)
			{
				curr_teb->ThreadLocalStoragePointer = tls;
				return -1;
			}
		}
		else
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[0][6];
			msghWnd = g_hWnd;
			script = g_script;
		}
	}
	if (!g_script || !script->mIsReadyToExecute)
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return -1; // AutoHotkey needs to be running at this point //
	}
	Func *aFunc = script->FindFunc(func) ;
	if (aFunc)
	{	
		int aParamsCount = 0;
		LPTSTR *params[10] = {&param1,&param2,&param3,&param4,&param5,&param6,&param7,&param8,&param9,&param10};
		for (;aParamsCount < 10;aParamsCount++)
			if (!*params[aParamsCount])
				break;
		if (aParamsCount < aFunc->mMinParams)
		{
			script->ScriptError(ERR_TOO_FEW_PARAMS, func);
			if (tls)
				curr_teb->ThreadLocalStoragePointer = tls;
			return -1;
		}
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		FuncAndToken & aFuncAndToken = g_FuncAndTokenToReturn[InterlockedIncrement16(&returnCount) & 0xFFF];
		INITFUNCANDTOKEN
		aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
		for (int i = 0;(aFunc->mParamCount > i || aFunc->mIsVariadic) && aParamsCount>i;i++)
			aFuncAndToken.params[i].var->Assign(*params[i]);
		aFuncAndToken.mFunc = aFunc;
		PostMessage(msghWnd, AHK_EXECUTE_FUNCTION, (WPARAM)&aFuncAndToken,NULL);
		return 0;
	} 
	else // Function not found
	{
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
		return -1;
	}
}


//H30 changed to not return anything since it is not used
void callFuncDll(FuncAndToken *aFuncAndToken)
{
 	Func &func =  *(aFuncAndToken->mFunc); 
	ResultToken & aResultToken = aFuncAndToken->mToken ;
	// Func &func = *(Func *)g_script->mTempFunc ;
	if (!INTERRUPTIBLE_IN_EMERGENCY)
		return;
	if (g_nThreads >= g_MaxThreadsTotal)
			return;

	// Need to check if backup is needed in case script explicitly called the function rather than using
	// it solely as a callback.  UPDATE: And now that max_instances is supported, also need it for that.
	// See ExpandExpression() for detailed comments about the following section.
	//VarBkp *var_backup = NULL;   // If needed, it will hold an array of VarBkp objects.
	//int var_backup_count; // The number of items in the above array.
	//if (func.mInstances > 0) // Backup is needed.
		//if (!Var::BackupFunctionVars(func, var_backup, var_backup_count)) // Out of memory.
			// return;
			// Since we're in the middle of processing messages, and since out-of-memory is so rare,
			// it seems justifiable not to have any error reporting and instead just avoid launching
			// the new thread.

	// Since above didn't return, the launch of the new thread is now considered unavoidable.

	// See MsgSleep() for comments about the following section.
	InitNewThread(0, false, true);

	//for (int aParamCount = 0;func.mParamCount > aParamCount && aFuncAndToken->mParamCount > aParamCount;aParamCount++)
	//	func.mParam[aParamCount].var->AssignString(aFuncAndToken->param[aParamCount]);

	// DEBUGGER_STACK_PUSH(&func) //HotKeyIt, AFAIK this is not necessary since it is done in func.Call internally
	FuncResult func_call;
	// func_call.CopyExprFrom(aResultToken);
	// ExprTokenType &aResultToken = aResultToken_to_return ;
	bool result = func.Call(func_call,aFuncAndToken->param,(int) aFuncAndToken->mParamCount); // Call the UDF.

	// DEBUGGER_STACK_POP() //HotKeyIt, AFAIK this is not necessary since it is done in func.Call internally
	LPTSTR new_buf;
	if (result)
	{
		switch (func_call.symbol)
		{
		case SYM_VAR: // Caller has ensured that any SYM_VAR's Type() is VAR_NORMAL.
			if (_tcslen(func_call.var->Contents()))
			{
				new_buf = (LPTSTR)realloc((LPTSTR)aFuncAndToken->result_to_return_dll, (_tcslen(func_call.var->Contents()) + 1)*sizeof(TCHAR));
				if (!new_buf)
				{
					g_script->ScriptError(ERR_OUTOFMEM,func.mName);
					return;
				}
				aFuncAndToken->result_to_return_dll = new_buf;
				_tcscpy(aFuncAndToken->result_to_return_dll, func_call.var->Contents()); // Contents() vs. mContents to support VAR_CLIPBOARD, and in case mContents needs to be updated by Contents().
			}
			else if (aFuncAndToken->result_to_return_dll)
				*aFuncAndToken->result_to_return_dll = '\0';
			break;
		case SYM_STRING:
			if (_tcslen(func_call.marker))
			{
				new_buf = (LPTSTR)realloc((LPTSTR)aFuncAndToken->result_to_return_dll, (_tcslen(func_call.marker) + 1)*sizeof(TCHAR));
				if (!new_buf)
				{
					g_script->ScriptError(ERR_OUTOFMEM,func.mName);
					return;
				}
				aFuncAndToken->result_to_return_dll = new_buf;
				_tcscpy(aFuncAndToken->result_to_return_dll, func_call.marker);
			}
			else if (aFuncAndToken->result_to_return_dll)
				*aFuncAndToken->result_to_return_dll = '\0';
			break;
		case SYM_INTEGER:
			new_buf = (LPTSTR )realloc((LPTSTR )aFuncAndToken->result_to_return_dll,MAX_INTEGER_LENGTH);
			if (!new_buf)
			{
				g_script->ScriptError(ERR_OUTOFMEM,func.mName);
				return;
			}
			aFuncAndToken->result_to_return_dll = new_buf;
			ITOA64(func_call.value_int64, aFuncAndToken->result_to_return_dll);
			break;
		case SYM_FLOAT:
			new_buf = (LPTSTR )realloc((LPTSTR )aFuncAndToken->result_to_return_dll,MAX_INTEGER_LENGTH);
			if (!new_buf)
			{
				g_script->ScriptError(ERR_OUTOFMEM,func.mName);
				return;
			}
			aFuncAndToken->result_to_return_dll = new_buf;
			FTOA(func_call.value_double, aFuncAndToken->result_to_return_dll, MAX_NUMBER_SIZE);
			break;
		//case SYM_OBJECT: // L31: Treat objects as empty strings (or TRUE where appropriate).
		default: // Not an operand: continue on to return the default at the bottom.
			if (aFuncAndToken->result_to_return_dll)
				*aFuncAndToken->result_to_return_dll = '\0';
		}
	}
	else if (aFuncAndToken->result_to_return_dll)
			*aFuncAndToken->result_to_return_dll = '\0';
	ResumeUnderlyingThread();
	return;
}

#ifndef _USRDLL
//void AssignVariant(Var &aArg, VARIANT &aVar, bool aRetainVar = true);
void AssignVariant(Var& aArg, VARIANT& aVar, bool aRetainVar = true);
VARIANT ahkFunctionVariant(LPTSTR func, VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10, int sendOrPost)
{
	UserFunc *aFunc = (UserFunc *)g_script->FindFunc(func);
	FuncAndToken &aFuncAndToken = g_FuncAndTokenToReturn[InterlockedIncrement16(&returnCount) & 0xFFF];
	INITFUNCANDTOKEN
	if (aFunc)
	{
		VARIANT* variants[10] = { &param1,&param2,&param3,&param4,&param5,&param6,&param7,&param8,&param9,&param10 };
		int aParamsCount = 0;
		for (; aParamsCount < 10; aParamsCount++)
			if (variants[aParamsCount]->vt == VT_ERROR)
				break;
		if (aParamsCount < aFunc->mMinParams)
		{
			g_script->ScriptError(ERR_TOO_FEW_PARAMS);
			aFuncAndToken.variant_to_return_dll.vt = VT_NULL;
			return aFuncAndToken.variant_to_return_dll;
		}
		for (int i = 0; aParamsCount > i; i++)
			AssignVariant(*aFuncAndToken.params[i].var, *variants[i], true);
		aFuncAndToken.mFunc = aFunc;
		aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
		if (sendOrPost)
		{
			callFuncDllVariant(&aFuncAndToken);
			return aFuncAndToken.variant_to_return_dll;
		}
		else
		{
			aFuncAndToken.variant_to_return_dll.vt = VT_BOOL;
			aFuncAndToken.variant_to_return_dll.boolVal = PostMessage(g_hWnd, AHK_EXECUTE_FUNCTION_VARIANT, (WPARAM)&aFuncAndToken, (LPARAM)NULL);
			return aFuncAndToken.variant_to_return_dll;
		}
	}
	aFuncAndToken.variant_to_return_dll.vt = VT_NULL ;
	return aFuncAndToken.variant_to_return_dll;
	// should return a blank variant
}

void callFuncDllVariant(FuncAndToken *aFuncAndToken)
{
 	UserFunc &func =  *(UserFunc *)aFuncAndToken->mFunc;
	ResultToken & aResultToken = aFuncAndToken->mToken ;
	// Func &func = *(Func *)g_script->mTempFunc ;
	if (!INTERRUPTIBLE_IN_EMERGENCY)
		return;
	if (g_nThreads >= g_MaxThreadsTotal)
			return;

	// Need to check if backup is needed in case script explicitly called the function rather than using
	// it solely as a callback.  UPDATE: And now that max_instances is supported, also need it for that.
	// See ExpandExpression() for detailed comments about the following section.
	VarBkp *var_backup = NULL;   // If needed, it will hold an array of VarBkp objects.
	int var_backup_count; // The number of items in the above array.
	if (func.mInstances > 0) // Backup is needed.
		if (!Var::BackupFunctionVars(func, var_backup, var_backup_count)) // Out of memory.
			return;
			// Since we're in the middle of processing messages, and since out-of-memory is so rare,
			// it seems justifiable not to have any error reporting and instead just avoid launching
			// the new thread.

	// Since above didn't return, the launch of the new thread is now considered unavoidable.

	// See MsgSleep() for comments about the following section.
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);
	UDFCallInfo recurse(&func);
	DEBUGGER_STACK_PUSH(&recurse)
	// ExprTokenType aResultToken;
	// ExprTokenType &aResultToken = aResultToken_to_return ;
	++func.mInstances;
	func.Call(aResultToken, aFuncAndToken->param, (int)aFuncAndToken->mParamCount); // Call the UDF.

	TokenToVariant(aResultToken, aFuncAndToken->variant_to_return_dll, FALSE);

	aResultToken.Free();
	Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count); // ABOVE must be done BEFORE this because return_value might be the contents of one of the function's local variables (which are about to be free'd).
	--func.mInstances;

	DEBUGGER_STACK_POP()
	
	ResumeUnderlyingThread();
	return;
}
#endif
