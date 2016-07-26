#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

LPTSTR result_to_return_dll; //HotKeyIt H2 for ahkgetvar and ahkFunction return.
VARIANT variant_to_return_dll;
// ExprTokenType aResultToken_to_return ;  // for ahkPostFunction
FuncAndToken aFuncAndTokenToReturn[10] ;    // for ahkPostFunction
int returnCount = -1 ;
void TokenToVariant(ExprTokenType &aToken, VARIANT &aVar, BOOL aVarIsArg);

// Following macros are used in addFile addScript ahkExec
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
			aLastHotExpr->NextCriterion = g_FirstHotExpr;\
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
	int aFuncCount = g_script->mFuncCount;\
	int aCurrFileIndex = g_script->mCurrFileIndex, aCombinedLineNumber = g_script->mCombinedLineNumber, aCurrentFuncOpenBlockCount = g_script->mCurrentFuncOpenBlockCount;\
	bool aNextLineIsFunctionBody = g_script->mNextLineIsFunctionBody;\
	Line *aFirstLine = g_script->mFirstLine,*aLastLine = g_script->mLastLine,*aCurrLine = g_script->mCurrLine,*aFirstStaticLine = g_script->mFirstStaticLine,*aLastStaticLine = g_script->mLastStaticLine;\
	g_script->mCurrentFuncOpenBlockCount = NULL;\
	g_script->mNextLineIsFunctionBody = false;\
	Func *aCurrFunc  = g->CurrentFunc;\
	Label *aPlaceholderLabel = g_script->mPlaceholderLabel;\
	int aClassObjectCount = g_script->mClassObjectCount;\
	g_script->mClassObjectCount = NULL;\
	g_script->mFirstStaticLine = NULL;g_script->mLastStaticLine = NULL;\
	g_script->mFirstLine = NULL;g_script->mLastLine = NULL;\
	g_script->mIsReadyToExecute = false;\
	g->CurrentFunc = NULL;

#define RESTORE_G_SCRIPT \
	g_script->mFirstLine = aFirstLine;\
	g_script->mLastLine = aLastLine;\
	g_script->mLastLine->mNextLine = NULL;\
	g_script->mCurrLine = aCurrLine;\
	g_script->mPlaceholderLabel = aPlaceholderLabel;\
	g_script->mClassObjectCount = aClassObjectCount + g_script->mClassObjectCount;\
	g_script->mCurrFileIndex = aCurrFileIndex;\
	g_script->mCurrentFuncOpenBlockCount = aCurrentFuncOpenBlockCount;\
	g_script->mNextLineIsFunctionBody = aNextLineIsFunctionBody;\
	g_script->mCombinedLineNumber = aCombinedLineNumber;
#ifdef _USRDLL
//COM virtual functions
int com_ahkPause(LPTSTR aChangeTo){return ahkPause(aChangeTo);}
UINT_PTR com_ahkFindLabel(LPTSTR aLabelName){return ahkFindLabel(aLabelName);}
// LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar){return ahkgetvar(name,getVar);}
// unsigned int com_ahkassign(LPTSTR name, LPTSTR value){return ahkassign(name,value);}
UINT_PTR com_ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait){return ahkExecuteLine(line,aMode,wait);}
int com_ahkLabel(LPTSTR aLabelName, unsigned int nowait){return ahkLabel(aLabelName,nowait);}
UINT_PTR com_ahkFindFunc(LPTSTR funcname){return ahkFindFunc(funcname);}
// LPTSTR com_ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10){return ahkFunction(func,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10);}
// unsigned int com_ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10){return ahkPostFunction(func,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10);}
UINT_PTR com_addScript(LPTSTR script, int waitexecute){return addScript(script,waitexecute);}
int com_ahkExec(LPTSTR script){return ahkExec(script);}
UINT_PTR com_addFile(LPTSTR fileName, int waitexecute){return addFile(fileName,waitexecute);}
UINT_PTR com_ahkdll(LPTSTR fileName,LPTSTR argv){return ahkdll(fileName,argv);}
UINT_PTR com_ahktextdll(LPTSTR script,LPTSTR argv){return ahktextdll(script,argv);}
int com_ahkTerminate(int timeout){return ahkTerminate(timeout);}
int com_ahkReady(){return ahkReady();}
int com_ahkIsUnicode(){return ahkIsUnicode();}
int com_ahkReload(int timeout){return ahkReload(timeout);}
#endif

EXPORT int ahkIsUnicode()
{
#ifdef UNICODE
	return true;
#else
	return false;
#endif
}

#ifdef _USRDLL
EXPORT int ahkReady() // HotKeyIt check if dll is ready to execute
#else
EXPORT int ahkReady(DWORD aThreadID) // HotKeyIt check if dll is ready to execute
#endif
{
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	bool IsReadyToExecute = false;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
	return IsReadyToExecute;
#else
	return (g_script && g_script->mIsReadyToExecute) || g_Reloading || g_Loading;
#endif
}

#ifdef _USRDLL
EXPORT int ahkPause(LPTSTR aChangeTo) //Change pause state of a running script
#else
EXPORT int ahkPause(LPTSTR aChangeTo, DWORD aThreadID) //Change pause state of a running script
#endif
{
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}

	if ( (((int)aChangeTo == 1 || (int)aChangeTo == 0) || (*aChangeTo == 'O' || *aChangeTo == 'o') && ( *(aChangeTo+1) == 'N' || *(aChangeTo+1) == 'n' ) ) || *aChangeTo == '1')
	{
		Hotkey::ResetRunAgainAfterFinished();
		if ((int)aChangeTo == 0 || ((int)aChangeTo != 1 && (*aChangeTo == '0' || (*(aChangeTo + 1) == 'F' || *(aChangeTo + 1) == 'f'))))
		{
			g->IsPaused = false;
			--g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
		}
		else
		{
			g->IsPaused = true;
			++g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
		}
		g_script->UpdateTrayIcon();
	}
	else if (*aChangeTo != '\0')
	{
		g->IsPaused = false;
		--g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
		g_script->UpdateTrayIcon();
	}
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return (int)g->IsPaused;
}

#ifdef _USRDLL
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname)
#else
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname, DWORD aThreadID)
#endif
{
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}
	UINT_PTR result = (UINT_PTR)g_script->FindFunc(funcname);
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return result;
}

#ifdef _USRDLL
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName)
#else
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName, DWORD aThreadID)
#endif
{
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}
	UINT_PTR result = (UINT_PTR)g_script->FindLabel(aLabelName);
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return result;
}

// Naveen: v1. ahkgetvar()
#ifdef _USRDLL
EXPORT LPTSTR ahkgetvar(LPTSTR name, unsigned int getVar)
#else
EXPORT LPTSTR ahkgetvar(LPTSTR name, unsigned int getVar, DWORD aThreadID)
#endif
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
#ifndef _USRDLL
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}
	if (g_ThreadID != ThreadID)
		SuspendThread(g_hThread);
	Var *ahkvar = g_script->FindOrAddVar(name, 0, VAR_GLOBAL);
	if (getVar != NULL)
	{
		if (ahkvar->mType == VAR_BUILTIN || ahkvar->mType == VAR_VIRTUAL)
		{
			if (g_ThreadID != ThreadID)
				ResumeThread(g_hThread);

#ifndef _USRDLL
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
#endif
			return _T("");
		}
		LPTSTR new_mem = (LPTSTR)realloc((LPTSTR )result_to_return_dll,MAX_INTEGER_LENGTH);
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (g_ThreadID != ThreadID)
				ResumeThread(g_hThread);
#ifndef _USRDLL
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
#endif
			return _T("");
		}
		result_to_return_dll = new_mem;
		if (g_ThreadID != ThreadID)
			ResumeThread(g_hThread);
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return ITOA64((UINT_PTR)ahkvar,result_to_return_dll);
	}
	else if (ahkvar->mType != VAR_BUILTIN && !ahkvar->HasContents() )
	{
		if (g_ThreadID != ThreadID)
			ResumeThread(g_hThread);
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return _T("");
	}
	if (*ahkvar->mCharContents == '\0')
	{
		LPTSTR new_mem = (LPTSTR )realloc((LPTSTR )result_to_return_dll,(ahkvar->mType == VAR_BUILTIN ? ahkvar->mBIV(0,name) : ahkvar->mByteCapacity ? ahkvar->mByteCapacity : ahkvar->mByteLength) + MAX_NUMBER_LENGTH + sizeof(TCHAR));
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (g_ThreadID != ThreadID)
				ResumeThread(g_hThread);
#ifndef _USRDLL
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
#endif
			return _T("");
		}
		result_to_return_dll = new_mem;
		if ( ahkvar->mType == VAR_BUILTIN )
		{
			if (ahkvar->mBIV == BIV_IsPaused)
			{
				++g; // imitate new thread for A_IsPaused
				ahkvar->mBIV(result_to_return_dll,name); //Hotkeyit 
				--g;
			}
			else
				ahkvar->mBIV(result_to_return_dll,name); //Hotkeyit 
		}
		else if ( ahkvar->mType == VAR_VIRTUAL)
			ahkvar->mVV->Get(result_to_return_dll, name);
		else if ( ahkvar->mType == VAR_ALIAS )
			ITOA64(ahkvar->mAliasFor->mContentsInt64,result_to_return_dll);
		else if ( ahkvar->mType == VAR_NORMAL )
			ITOA64(ahkvar->mContentsInt64,result_to_return_dll);//Hotkeyit
	}
	else
	{
		LPTSTR new_mem = (LPTSTR)realloc((LPTSTR)result_to_return_dll, ahkvar->mType == VAR_BUILTIN ? ahkvar->mBIV(0, name) : (ahkvar->mType == VAR_VIRTUAL ? ahkvar->mVV->Get(0, name) : ahkvar->mByteLength + sizeof(TCHAR)));
		if (!new_mem)
		{
			g_script->ScriptError(ERR_OUTOFMEM, name);
			if (g_ThreadID != ThreadID)
				ResumeThread(g_hThread);
#ifndef _USRDLL
			if (curr_teb)
				curr_teb->ThreadLocalStoragePointer = tls;
#endif
			return _T("");
		}
		result_to_return_dll = new_mem;
		if ( ahkvar->mType == VAR_ALIAS )
			ahkvar->mAliasFor->Get(result_to_return_dll); //Hotkeyit removed ebiv.cpp and made ahkgetvar return all vars
 		else if ( ahkvar->mType == VAR_NORMAL )
			ahkvar->Get(result_to_return_dll);  // var.getText() added in V1.
		else if ( ahkvar->mType == VAR_BUILTIN )
			ahkvar->mBIV(result_to_return_dll,name); //Hotkeyit 
		else if (ahkvar->mType == VAR_VIRTUAL)
			ahkvar->mVV->Get(result_to_return_dll, name); //Hotkeyit 
	}
	if (g_ThreadID != ThreadID)
		ResumeThread(g_hThread);
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return result_to_return_dll;
}	

#ifdef _USRDLL
EXPORT int ahkassign(LPTSTR name, LPTSTR value) // ahkwine 0.1
#else
EXPORT int ahkassign(LPTSTR name, LPTSTR value, DWORD aThreadID) // ahkwine 0.1
#endif
{
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
#ifndef _USRDLL
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return -1; // AutoHotkey needs to be running at this point //
	}
	if (g_ThreadID != ThreadID)
		SuspendThread(g_hThread);
	Var *var;
	if (!(var = g_script->FindOrAddVar(name, 0, VAR_GLOBAL)))
	{
		if (g_ThreadID != ThreadID)
			ResumeThread(g_hThread);
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return -1;  // Realistically should never happen.
	}
	var->Assign(value);
	if (g_ThreadID != ThreadID)
		ResumeThread(g_hThread);
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return 0; // success
}

//HotKeyIt ahkExecuteLine()
#ifdef _USRDLL
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line, unsigned int aMode, unsigned int wait)
#else
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line, unsigned int aMode, unsigned int wait, DWORD aThreadID)
#endif
{
	HWND msghWnd = g_hWnd;
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}
	Line *templine = (Line *)line;
	if (templine == NULL)
	{
		UINT_PTR result = (UINT_PTR)g_script->mFirstLine;
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return result;
	}
#ifndef _USRDLL
	msghWnd = g_hWnd;
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
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

#ifdef _USRDLL
EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait) // 0 = wait = default
#else
EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait, DWORD aThreadID) // 0 = wait = default
#endif
{
	HWND msghWnd = g_hWnd;
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point //
	}
	Label *aLabel = g_script->FindLabel(aLabelName) ;
	if (aLabel)
	{
#ifndef _USRDLL
		msghWnd = g_hWnd;
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		if (nowait)
			PostMessage(msghWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		else
			SendMessage(msghWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		return 1;
	}
	else
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0;
	}
}

#ifdef _USRDLL
EXPORT int ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10)
#else
EXPORT int ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10, DWORD aThreadID)
#endif
{
	HWND msghWnd = g_hWnd;
	Script *script = g_script;
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
	{
		PMYTEB curr_teb = NULL;
		PVOID tls = NULL;
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
					curr_teb->ThreadLocalStoragePointer = tls;
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
			msghWnd = g_hWnd;
			script = g_script;
			curr_teb->ThreadLocalStoragePointer = tls;
		}
	}
#endif
	if (!g_script || !script->mIsReadyToExecute)
	{
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
			return -1;
		}
		if(aFunc->mIsBuiltIn)
		{
			EnterCriticalSection(&g_CriticalAhkFunction);
			ResultType aResult = OK;
			if (++returnCount > 9)
				returnCount = 0 ;
			FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
			if (aParamsCount)
			{
				ExprTokenType **new_mem = (ExprTokenType**)realloc(aFuncAndToken.param,sizeof(ExprTokenType)*aParamsCount);
				if (!new_mem)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return -1;
				}
				aFuncAndToken.param = new_mem;
			}
			else
				aFuncAndToken.param = NULL;
			for (int i = 0;aFunc->mParamCount > i && aParamsCount>i;i++)
			{
				aFuncAndToken.param[i] = &aFuncAndToken.params[i];
				aFuncAndToken.param[i]->SetValue(*params[i]); // Assign parameters
			}
			aFuncAndToken.mToken.symbol = SYM_INTEGER;
			LPTSTR new_buf = (LPTSTR)realloc(aFuncAndToken.buf,MAX_NUMBER_SIZE * sizeof(TCHAR));
			if (!new_buf)
			{
				script->ScriptError(ERR_OUTOFMEM, func);
				LeaveCriticalSection(&g_CriticalAhkFunction);
				return -1;
			}
			aFuncAndToken.buf = new_buf;
			aFuncAndToken.mToken.buf = aFuncAndToken.buf;
			aFuncAndToken.mToken.func = aFunc;
			aFuncAndToken.mToken.marker = aFunc->mName;
			
			aFunc->mBIF(aFuncAndToken.mToken,aFuncAndToken.param,aParamsCount);
			LeaveCriticalSection(&g_CriticalAhkFunction);
			return 0;
		}
		else
		{
			EnterCriticalSection(&g_CriticalAhkFunction);
			if (++returnCount > 9)
				returnCount = 0 ;
			FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
			if (aParamsCount)
			{
				ExprTokenType **new_mem = (ExprTokenType**)realloc(aFuncAndToken.param,sizeof(ExprTokenType)*aParamsCount);
				if (!new_mem)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return -1;
				}
				aFuncAndToken.param = new_mem;
			}
			else
				aFuncAndToken.param = NULL;
			aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
			LPTSTR new_buf;
			for (int i = 0;(aFunc->mParamCount > i || aFunc->mIsVariadic) && aParamsCount>i;i++)
			{
				aFuncAndToken.param[i] = &aFuncAndToken.params[i];
				new_buf = (LPTSTR)realloc(aFuncAndToken.param[i]->marker,(_tcslen(*params[i])+1)*sizeof(TCHAR));
				if (!new_buf)
				{
					script->ScriptError(ERR_OUTOFMEM, func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return -1;
				}
				_tcscpy(new_buf,*params[i]); // Assign parameters
				aFuncAndToken.param[i]->SetValue(new_buf);
			}
			aFuncAndToken.mFunc = aFunc ;
			PostMessage(msghWnd, AHK_EXECUTE_FUNCTION_DLL, (WPARAM)&aFuncAndToken,NULL);
			LeaveCriticalSection(&g_CriticalAhkFunction);
			return 0;
		}
	} 
	else // Function not found
	{
		return -1;
	}
}

// Naveen: v6 addFile()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
#ifdef _USRDLL
EXPORT UINT_PTR addFile(LPTSTR fileName, int waitexecute)
#else
EXPORT UINT_PTR addFile(LPTSTR fileName, int waitexecute, DWORD aThreadID)
#endif
{   // dynamically include a file into a script !!
	// labels, hotkeys, functions. 
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point // LOADING_FAILED cant be used due to PTR return type
	}
	int HotkeyCount = Hotkey::sHotkeyCount;
	HotkeyCriterion *aFirstHotExpr = g_FirstHotExpr,*aLastHotExpr = g_LastHotExpr;
	g_FirstHotExpr = NULL;g_LastHotExpr = NULL;
	GuiType *aGuiDefaultWindow = g->GuiDefaultWindow;
	g->GuiDefaultWindow = NULL;
	//int a_guiCount = g_guiCount;
	//g_guiCount = 0;
#ifdef _USRDLL
	g_Loading = true;
#endif
	BACKUP_G_SCRIPT
	LPTSTR oldFileSpec = g_script->mFileSpec;
	g_script->mFileSpec = fileName;
	if (g_script->LoadFromFile()!= OK) //fileName, aAllowDuplicateInclude, (bool) aIgnoreLoadFailure) != OK) || !g_script->PreparseBlocks(oldLastLine->mNextLine))
	{
		g_script->mFileSpec = oldFileSpec;				// Restore script path
		g->CurrentFunc = aCurrFunc;						// Restore current function
		if (g_script->mPlaceholderLabel)
			delete g_script->mPlaceholderLabel;
		RESTORE_G_SCRIPT
		g->GuiDefaultWindow = aGuiDefaultWindow;
		//g_guiCount = a_guiCount;
		RESTORE_IF_EXPR
		g_script->mIsReadyToExecute = true; // Set program to be ready for continuing previous script.
#ifdef _USRDLL
		g_Loading = false;
#endif
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // LOADING_FAILED cant be used due to PTR return type
	}	
	g_script->mFileSpec = oldFileSpec;
	g->GuiDefaultWindow = aGuiDefaultWindow;
	//g_guiCount = a_guiCount;
	FINALIZE_HOTKEYS
	RESTORE_IF_EXPR
	g_script->mIsReadyToExecute = true;
#ifdef _USRDLL
	g_Loading = false;
#endif
	g->CurrentFunc = aCurrFunc;
	if (waitexecute != 0)
	{
		if (waitexecute == 1)
		{
			g_ReturnNotExit = true;
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script->mFirstLine, (LPARAM)NULL);
		}
		else
			PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script->mFirstLine, (LPARAM)NULL);
		g_ReturnNotExit = false;
	}
	else
	{  // Static init lines need always to run
		Line *tempstatic = NULL;
		while (tempstatic != g_script->mLastStaticLine)
		{
			if (tempstatic == NULL)
				tempstatic = g_script->mFirstStaticLine;
			else
				tempstatic = tempstatic->mNextLine;
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)tempstatic, (LPARAM)ONLY_ONE_LINE);
		}
	}
	Line *aTempLine = g_script->mFirstLine; // required for return
	aLastLine->mNextLine = aTempLine;
	aTempLine->mPrevLine = aLastLine;
	aLastLine = g_script->mLastLine;
	delete g_script->mPlaceholderLabel;
	RESTORE_G_SCRIPT
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return (UINT_PTR) aTempLine;
}

// HotKeyIt: addScript()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
#ifdef _USRDLL
EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute)
#else
EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute, DWORD aThreadID)
#endif
{   // dynamically include a script from text!!
	// labels, hotkeys, functions.
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point // LOADING_FAILED cant be used due to PTR return type
	}
	int HotkeyCount = Hotkey::sHotkeyCount;
	HotkeyCriterion *aFirstHotExpr = g_FirstHotExpr,*aLastHotExpr = g_LastHotExpr;
	g_FirstHotExpr = NULL;g_LastHotExpr = NULL;
	GuiType *aGuiDefaultWindow = g->GuiDefaultWindow;
	g->GuiDefaultWindow = NULL;
	//int a_guiCount = g_guiCount;
	//g_guiCount = 0;

	LPCTSTR aPathToShow = g_script->mCurrLine->mArg ? g_script->mCurrLine->mArg->text : g_script->mFileSpec;
#ifdef _USRDLL
	g_Loading = true;
#endif
	BACKUP_G_SCRIPT
	if (g_script->LoadFromText(script,aPathToShow) != OK) // || !g_script->PreparseBlocks(oldLastLine->mNextLine)))
	{
		g->CurrentFunc = aCurrFunc;
		if (g_script->mPlaceholderLabel)
			delete g_script->mPlaceholderLabel;
		RESTORE_G_SCRIPT
		g->GuiDefaultWindow = aGuiDefaultWindow;
		//g_guiCount = a_guiCount;
		RESTORE_IF_EXPR
		g_script->mIsReadyToExecute = true;
#ifdef _USRDLL
		g_Loading = false;
#endif
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0;  // LOADING_FAILED cant be used due to PTR return type
	}
	g->GuiDefaultWindow = aGuiDefaultWindow;
	//g_guiCount = a_guiCount;
	FINALIZE_HOTKEYS
	RESTORE_IF_EXPR
	g_script->mIsReadyToExecute = true;
#ifdef _USRDLL
	g_Loading = false;
#endif
	g->CurrentFunc = aCurrFunc;
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
	else
	{  // Static init lines need always to run
		Line *tempstatic = NULL;
		while (tempstatic != g_script->mLastStaticLine)
		{
			if (tempstatic == NULL)
				tempstatic = g_script->mFirstStaticLine;
			else
				tempstatic = tempstatic->mNextLine;
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)tempstatic, (LPARAM)ONLY_ONE_LINE);
		}
	}
	Line *aTempLine = g_script->mFirstLine;
	aLastLine->mNextLine = aTempLine;
	aTempLine->mPrevLine = aLastLine;
	aLastLine = g_script->mLastLine;
	delete g_script->mPlaceholderLabel;
	RESTORE_G_SCRIPT
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return (UINT_PTR) aTempLine;
}

// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
#ifdef _USRDLL
EXPORT int ahkExec(LPTSTR script)
#else
EXPORT int ahkExec(LPTSTR script, DWORD aThreadID)
#endif
{   // dynamically include a script from text!!
	// labels, hotkeys, functions
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
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
#endif
	if (!g_script || !g_script->mIsReadyToExecute)
	{
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return 0; // AutoHotkey needs to be running at this point // LOADING_FAILED cant be used due to PTR return type.
	}
	int HotkeyCount = Hotkey::sHotkeyCount;
	HotkeyCriterion *aFirstHotExpr = g_FirstHotExpr,*aLastHotExpr = g_LastHotExpr;
	g_FirstHotExpr = NULL;g_LastHotExpr = NULL;
#ifdef _USRDLL
	g_Loading = true;
#endif
	BACKUP_G_SCRIPT
	int aSourceFileIdx = Line::sSourceFileCount;

	// Backup SimpleHeap to restore later
	SimpleHeap *aSimpleHeap = new SimpleHeap();
	SimpleHeap *bkpSimpleHeap = g_SimpleHeap;
	g_SimpleHeap = aSimpleHeap;

	if ((g_script->LoadFromText(script) != OK)) // || !g_script->PreparseBlocks(oldLastLine->mNextLine))
	{
		g->CurrentFunc = aCurrFunc;
		if (g_script->mPlaceholderLabel)
			delete g_script->mPlaceholderLabel;
		// Delete used and restore SimpleHeap
		g_SimpleHeap = bkpSimpleHeap;
		aSimpleHeap->DeleteAll();
		delete aSimpleHeap;
		RESTORE_G_SCRIPT
		RESTORE_IF_EXPR
		g_script->mIsReadyToExecute = true;
#ifdef _USRDLL
		g_Loading = false;
#endif
#ifndef _USRDLL
		if (curr_teb)
			curr_teb->ThreadLocalStoragePointer = tls;
#endif
		return NULL;
	}
	FINALIZE_HOTKEYS
	RESTORE_IF_EXPR
	g_script->mIsReadyToExecute = true;
#ifdef _USRDLL
	g_Loading = false;
#endif
	g->CurrentFunc = aCurrFunc;
	Line *aTempLine = g_script->mLastLine;
	Line *aExecLine = g_script->mFirstLine;
	delete g_script->mPlaceholderLabel;
	RESTORE_G_SCRIPT
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
	for (;Line::sSourceFileCount>aSourceFileIdx;)
		if (Line::sSourceFile[--Line::sSourceFileCount] != g_script->mOurEXE)
			free(Line::sSourceFile[Line::sSourceFileCount]);
	// Delete used and restore SimpleHeap
	aSimpleHeap->DeleteAll();
	delete aSimpleHeap;
#ifndef _USRDLL
	if (curr_teb)
		curr_teb->ThreadLocalStoragePointer = tls;
#endif
	return OK;
}

#ifdef _USRDLL
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10)
#else
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10, DWORD aThreadID)
#endif
{
	HWND msghWnd = g_hWnd;
	Script *script = g_script;
#ifndef _USRDLL
#ifdef _WIN64
	DWORD ThreadID = __readgsdword(0x48);
#else
	DWORD ThreadID = __readfsdword(0x24);
#endif
	if (g_ThreadID != ThreadID || (aThreadID && aThreadID != g_ThreadID))
	{
		PMYTEB curr_teb = NULL;
		PVOID tls = NULL;
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
					curr_teb->ThreadLocalStoragePointer = tls;
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
			msghWnd = g_hWnd;
			script = g_script;
			curr_teb->ThreadLocalStoragePointer = tls;
		}
	}
#endif
	if (!g_script || !script->mIsReadyToExecute)
	{
		return 0; // AutoHotkey needs to be running at this point //
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
			return _T("");
		}
		if(aFunc->mIsBuiltIn)
		{
			ResultType aResult = OK;
			EnterCriticalSection(&g_CriticalAhkFunction);
			if (++returnCount > 9)
				returnCount = 0 ;
			FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
			if (aParamsCount)
			{
				ExprTokenType **new_mem = (ExprTokenType**)realloc(aFuncAndToken.param,sizeof(ExprTokenType)*aParamsCount);
				if (!new_mem)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				aFuncAndToken.param = new_mem;
			}
			else
				aFuncAndToken.param = NULL;
			for (int i = 0;aFunc->mParamCount > i && aParamsCount>i;i++)
			{
				aFuncAndToken.param[i] = &aFuncAndToken.params[i];
				aFuncAndToken.param[i]->SetValue(*params[i]); // Assign parameters
			}
			aFuncAndToken.mToken.symbol = SYM_INTEGER;
			LPTSTR new_buf = (LPTSTR)realloc(aFuncAndToken.buf,MAX_NUMBER_SIZE * sizeof(TCHAR));
			if (!new_buf)
			{
				script->ScriptError(ERR_OUTOFMEM,func);
				LeaveCriticalSection(&g_CriticalAhkFunction);
				return _T("");
			}
			aFuncAndToken.buf = new_buf;
			aFuncAndToken.mToken.buf = aFuncAndToken.buf;
			aFuncAndToken.mToken.func = aFunc;
			aFuncAndToken.mToken.marker = aFunc->mName;
			
			aFunc->mBIF(aFuncAndToken.mToken,aFuncAndToken.param,aParamsCount);
			
			switch (aFuncAndToken.mToken.symbol)
			{
			case SYM_VAR: // Caller has ensured that any SYM_VAR's Type() is VAR_NORMAL.
				if (_tcslen(aFuncAndToken.mToken.var->Contents()))
				{
					new_buf = (LPTSTR )realloc((LPTSTR )result_to_return_dll,(_tcslen(aFuncAndToken.mToken.var->Contents()) + 1)*sizeof(TCHAR));
					if (!new_buf)
					{
						script->ScriptError(ERR_OUTOFMEM,func);
						LeaveCriticalSection(&g_CriticalAhkFunction);
						return _T("");
					}
					result_to_return_dll = new_buf;
					_tcscpy(result_to_return_dll,aFuncAndToken.mToken.var->Contents()); // Contents() vs. mContents to support VAR_CLIPBOARD, and in case mContents needs to be updated by Contents().
				}
				else if (result_to_return_dll)
					*result_to_return_dll = '\0';
				break;
			case SYM_STRING:
				if (_tcslen(aFuncAndToken.mToken.marker))
				{
					new_buf = (LPTSTR )realloc((LPTSTR )result_to_return_dll,(_tcslen(aFuncAndToken.mToken.marker) + 1)*sizeof(TCHAR));
					if (!new_buf)
					{
						script->ScriptError(ERR_OUTOFMEM,func);
						LeaveCriticalSection(&g_CriticalAhkFunction);
						return _T("");
					}
					result_to_return_dll = new_buf;
					_tcscpy(result_to_return_dll,aFuncAndToken.mToken.marker);
				}
				else if (result_to_return_dll)
					*result_to_return_dll = '\0';
				break;
			case SYM_INTEGER:
				new_buf = (LPTSTR )realloc((LPTSTR )result_to_return_dll,MAX_INTEGER_LENGTH);
				if (!new_buf)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				result_to_return_dll = new_buf;
				ITOA64(aFuncAndToken.mToken.value_int64, result_to_return_dll);
				break;
			case SYM_FLOAT:
				new_buf = (LPTSTR )realloc((LPTSTR )result_to_return_dll,MAX_INTEGER_LENGTH);
				if (!new_buf)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				result_to_return_dll = new_buf;
				FTOA(aFuncAndToken.mToken.value_double, result_to_return_dll, MAX_NUMBER_SIZE);
				break;
			//case SYM_OBJECT: // L31: Treat objects as empty strings (or TRUE where appropriate).
			default: // Not an operand: continue on to return the default at the bottom.
				new_buf = (LPTSTR )realloc((LPTSTR )result_to_return_dll,MAX_INTEGER_LENGTH);
				if (!new_buf)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				result_to_return_dll = new_buf;
				ITOA64(aFuncAndToken.mToken.value_int64, result_to_return_dll);
			}
			LeaveCriticalSection(&g_CriticalAhkFunction);
			return result_to_return_dll;
		}
		else // UDF
		{
			//for (;aFunc->mParamCount > aParamCount && aParamsCount>aParamCount;aParamCount++)
			//	aFunc->mParam[aParamCount].var->AssignString(*params[aParamCount]);
			EnterCriticalSection(&g_CriticalAhkFunction);
			if (++returnCount > 9)
				returnCount = 0 ;
			FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
			if (aParamsCount)
			{
				ExprTokenType **new_mem = (ExprTokenType**)realloc(aFuncAndToken.param,sizeof(ExprTokenType)*aParamsCount);
				if (!new_mem)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				aFuncAndToken.param = new_mem;
			}
			else
				aFuncAndToken.param = NULL;
			aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
			LPTSTR new_buf;
			for (int i = 0;(aFunc->mParamCount > i || aFunc->mIsVariadic) && aParamsCount>i;i++)
			{
				aFuncAndToken.param[i] = &aFuncAndToken.params[i];
				new_buf = (LPTSTR)realloc(aFuncAndToken.param[i]->marker,(_tcslen(*params[i])+1)*sizeof(TCHAR));
				if (!new_buf)
				{
					script->ScriptError(ERR_OUTOFMEM,func);
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return _T("");
				}
				_tcscpy(new_buf,*params[i]); // Assign parameters
				aFuncAndToken.param[i]->SetValue(new_buf);
			}
			aFuncAndToken.mFunc = aFunc ;
			SendMessage(msghWnd, AHK_EXECUTE_FUNCTION_DLL, (WPARAM)&aFuncAndToken, NULL);
			LeaveCriticalSection(&g_CriticalAhkFunction);
			return aFuncAndToken.result_to_return_dll;
		}
	}
	else // Function not found
	{
		return _T("");
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
		// Below: Only a subset of ACT_IS_ALWAYS_ALLOWED is done here because:
		// 1) The omitted action types seem too obscure to grant always-run permission for msg-monitor events.
		// 2) Reduction in code size.
		if (g_nThreads >= MAX_THREADS_EMERGENCY // To avoid array overflow, this limit must by obeyed except where otherwise documented.
			|| func.mJumpToLine->mActionType != ACT_EXITAPP && func.mJumpToLine->mActionType != ACT_RELOAD)
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
	VarBkp ErrorLevel_saved;
	g_ErrorLevel->Backup(ErrorLevel_saved); // Back up the current ErrorLevel for later restoration.
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);

	//for (int aParamCount = 0;func.mParamCount > aParamCount && aFuncAndToken->mParamCount > aParamCount;aParamCount++)
	//	func.mParam[aParamCount].var->AssignString(aFuncAndToken->param[aParamCount]);

	DEBUGGER_STACK_PUSH(&func)
	FuncResult func_call;
	// func_call.CopyExprFrom(aResultToken);
	// ExprTokenType &aResultToken = aResultToken_to_return ;
	bool result = func.Call(func_call,aFuncAndToken->param,(int) aFuncAndToken->mParamCount,false); // Call the UDF.

	DEBUGGER_STACK_POP()
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
			result_to_return_dll = new_buf;
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
	ResumeUnderlyingThread(ErrorLevel_saved);
	return;
}

#ifdef _USRDLL
void AssignVariant(Var &aArg, VARIANT &aVar, bool aRetainVar = true);
VARIANT ahkFunctionVariant(LPTSTR func, VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10, int sendOrPost)
{
	Func *aFunc = g_script->FindFunc(func) ;
	if (aFunc)
	{	
		VARIANT *variants[10] = {&param1,&param2,&param3,&param4,&param5,&param6,&param7,&param8,&param9,&param10};
		int aParamsCount = 0;
		for (;aParamsCount < 10;aParamsCount++)
			if (variants[aParamsCount]->vt == VT_ERROR)
				break;
		if (aParamsCount < aFunc->mMinParams)
		{
			g_script->ScriptError(ERR_TOO_FEW_PARAMS);
			VARIANT &r =  aFuncAndTokenToReturn[returnCount + 1].variant_to_return_dll;
			r.vt = VT_NULL ;
			return r ; 
		}
		if(aFunc->mIsBuiltIn)
		{
			ResultType aResult = OK;
			EnterCriticalSection(&g_CriticalAhkFunction);
			ResultToken aResultToken;
			ExprTokenType **aParam = (ExprTokenType**)_alloca(sizeof(ExprTokenType)*10);
			if (!aParam)
			{
				g_script->ScriptError(ERR_OUTOFMEM,func);
				VARIANT ret = {};
				ret.vt = NULL;
				LeaveCriticalSection(&g_CriticalAhkFunction);
				return ret;
			}
			for (int i = 0;aFunc->mParamCount > i && aParamsCount>i;i++)
			{
				aParam[i] = (ExprTokenType*)_alloca(sizeof(ExprTokenType));
				if (!aParam[i])
				{
					VARIANT ret = {};
					ret.vt = NULL;
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return ret;
				}
				aParam[i]->symbol = SYM_VAR;
				aParam[i]->var = (Var*)alloca(sizeof(Var));
				if (!aParam[i]->var)
				{
					VARIANT ret = {};
					ret.vt = NULL;
					LeaveCriticalSection(&g_CriticalAhkFunction);
					return ret;
				}
				// prepare variable
				aParam[i]->var->mType = VAR_NORMAL;
				aParam[i]->var->mAttrib = 0;
				aParam[i]->var->mByteCapacity = 0;
				aParam[i]->var->mHowAllocated = ALLOC_MALLOC;
				
				AssignVariant(*aParam[i]->var, *variants[i],false);
			}
			aResultToken.symbol = SYM_INTEGER;
			aResultToken.marker = aFunc->mName;
			
			aFunc->mBIF(aResultToken,aParam,aParamsCount);

			// free all variables in case memory was allocated
			for (int i = 0;i < aParamsCount;i++)
				aParam[i]->var->Free();
			TokenToVariant(aResultToken, variant_to_return_dll, FALSE);
			LeaveCriticalSection(&g_CriticalAhkFunction);
			return variant_to_return_dll;
		}
		else // UDF
		{
			EnterCriticalSection(&g_CriticalAhkFunction);
			for (int i = 0;aFunc->mParamCount > i;i++)
				AssignVariant(*aFunc->mParam[i].var, *variants[i],false);
			if (++returnCount > 9)
				returnCount = 0 ;
			FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
			aFuncAndToken.mFunc = aFunc ;
			aFuncAndToken.mParamCount = aFunc->mParamCount < aParamsCount && !aFunc->mIsVariadic ? aFunc->mParamCount : aParamsCount;
			if (sendOrPost == 1)
			{
				SendMessage(g_hWnd, AHK_EXECUTE_FUNCTION_VARIANT, (WPARAM)&aFuncAndToken, NULL);
				LeaveCriticalSection(&g_CriticalAhkFunction);
				return aFuncAndToken.variant_to_return_dll;
			}
			else
			{
				PostMessage(g_hWnd, AHK_EXECUTE_FUNCTION_VARIANT, (WPARAM)&aFuncAndToken,NULL);
				VARIANT &r =  aFuncAndToken.variant_to_return_dll;
				r.vt = VT_NULL ;
				LeaveCriticalSection(&g_CriticalAhkFunction);
				return r ; 
			}
		}
	}
	FuncAndToken & aFuncAndToken = aFuncAndTokenToReturn[returnCount];
	VARIANT &r =  aFuncAndToken.variant_to_return_dll;
	r.vt = VT_NULL ;
	return r ; 
	// should return a blank variant
}

void callFuncDllVariant(FuncAndToken *aFuncAndToken)
{
 	Func &func =  *(aFuncAndToken->mFunc); 
	ResultToken & aResultToken = aFuncAndToken->mToken ;
	// Func &func = *(Func *)g_script->mTempFunc ;
	if (!INTERRUPTIBLE_IN_EMERGENCY)
		return;
	if (g_nThreads >= g_MaxThreadsTotal)
		// Below: Only a subset of ACT_IS_ALWAYS_ALLOWED is done here because:
		// 1) The omitted action types seem too obscure to grant always-run permission for msg-monitor events.
		// 2) Reduction in code size.
		if (g_nThreads >= MAX_THREADS_EMERGENCY // To avoid array overflow, this limit must by obeyed except where otherwise documented.
			|| func.mJumpToLine->mActionType != ACT_EXITAPP && func.mJumpToLine->mActionType != ACT_RELOAD)
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
	VarBkp ErrorLevel_saved;
	g_ErrorLevel->Backup(ErrorLevel_saved);
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);

	DEBUGGER_STACK_PUSH(&func)
	// ExprTokenType aResultToken;
	// ExprTokenType &aResultToken = aResultToken_to_return ;
	++func.mInstances;
	func.Call(&aResultToken); // Call the UDF.

	TokenToVariant(aResultToken, aFuncAndToken->variant_to_return_dll, FALSE);

	aResultToken.Free();
	Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count); // ABOVE must be done BEFORE this because return_value might be the contents of one of the function's local variables (which are about to be free'd).
	--func.mInstances;

	DEBUGGER_STACK_POP()
	
	ResumeUnderlyingThread(ErrorLevel_saved);
	return;
}
#endif
