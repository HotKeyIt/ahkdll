#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

static LPTSTR result_to_return_dll; //HotKeyIt H2 for ahkgetvar and ahkFunction return.

EXPORT int ahkPause(LPTSTR aChangeTo) //Change pause state of a running script
{
	if ( ( (*aChangeTo == 'O' || *aChangeTo == 'o') && ( *(aChangeTo+1) == 'N' || *(aChangeTo+1) == 'n' ) ) || *aChangeTo == '1')
	{
#ifndef MINIDLL
		Hotkey::ResetRunAgainAfterFinished();
#endif
		g->IsPaused = true;
		++g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
#ifndef MINIDLL
		g_script.UpdateTrayIcon();
#endif
	}
	else if (*aChangeTo != '\0')
	{
		g->IsPaused = false;
		--g_nPausedThreads; // For this purpose the idle thread is counted as a paused thread.
#ifndef MINIDLL
		g_script.UpdateTrayIcon();
#endif
	}
	return (int)g->IsPaused;
}


EXPORT unsigned int ahkFindFunc(LPTSTR funcname)
{
	return (unsigned int)g_script.FindFunc(funcname);
}

EXPORT unsigned int ahkFindLabel(LPTSTR aLabelName)
{
	Label *aLabel = g_script.FindLabel(aLabelName) ;
	return (unsigned int)aLabel->mJumpToLine;
}

EXPORT int ximportfunc(ahkx_int_str func1, ahkx_int_str func2, ahkx_int_str_str func3) // Naveen ahkx N11
{
    g_script.xifwinactive = func1 ;
    g_script.xwingetid  = func2 ;
    g_script.xsend = func3;
    return 0;
}

void BIF_FindFunc(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount) // Added in Nv8.
{
	// Set default return value in case of early return.
	aResultToken.symbol = SYM_INTEGER ;
	aResultToken.marker = _T("");
	// Get the first arg, which is the string used as the source of the extraction. Call it "findfunc" for clarity.
	TCHAR funcname_buf[MAX_NUMBER_SIZE]; // A separate buf because aResultToken.buf is sometimes used to store the result.
	LPTSTR funcname = TokenToString(*aParam[0], funcname_buf); // Remember that aResultToken.buf is part of a union, though in this case there's no danger of overwriting it since our result will always be of STRING type (not int or float).
	int funcname_length = (int)EXPR_TOKEN_LENGTH(aParam[0], funcname);
	aResultToken.value_int64 = (__int64)ahkFindFunc(funcname);
	return;
}
// Naveen: v1. ahkgetvar()
EXPORT LPTSTR ahkgetvar(LPTSTR name,unsigned int getVar)
{
	Var *ahkvar = g_script.FindOrAddVar(name);
	if (getVar != NULL)
	{
		if (ahkvar->mType == VAR_BUILTIN)
			return _T("");
		result_to_return_dll = (LPTSTR )realloc((LPTSTR )result_to_return_dll,MAX_INTEGER_LENGTH);
		return ITOA64((int)ahkvar,result_to_return_dll);;
	}
	if (!ahkvar->HasContents() && ahkvar->mType != VAR_BUILTIN )
		return _T("");
	if (*ahkvar->mCharContents == '\0')
	{
		result_to_return_dll = (LPTSTR )realloc((LPTSTR )result_to_return_dll,(ahkvar->mByteCapacity ? ahkvar->mByteCapacity : ahkvar->mByteLength) + MAX_NUMBER_LENGTH + 1);
		if ( ahkvar->mType == VAR_BUILTIN )
			ahkvar->mBIV(result_to_return_dll,name); //Hotkeyit 
		else if ( ahkvar->mType == VAR_ALIAS )
			ITOA64(ahkvar->mAliasFor->mContentsInt64,result_to_return_dll);
		else if ( ahkvar->mType == VAR_NORMAL )
			ITOA64(ahkvar->mContentsInt64,result_to_return_dll);//Hotkeyit
	}
	else
	{
		result_to_return_dll = (LPTSTR )realloc((LPTSTR )result_to_return_dll,ahkvar->mByteLength+1);
		if ( ahkvar->mType == VAR_ALIAS )
			ahkvar->mAliasFor->Get(result_to_return_dll); //Hotkeyit removed ebiv.cpp and made ahkgetvar return all vars
 		else if ( ahkvar->mType == VAR_NORMAL )
			ahkvar->Get(result_to_return_dll);  // var.getText() added in V1.
		else if ( ahkvar->mType == VAR_BUILTIN )
			ahkvar->mBIV(result_to_return_dll,name); //Hotkeyit 
	}
	return result_to_return_dll;
}	

EXPORT int ahkassign(LPTSTR name, LPTSTR value) // ahkwine 0.1
{
	Var *var;
	if (   !(var = g_script.FindOrAddVar(name, _tcslen(name)))   )
		return -1;  // Realistically should never happen.
	var->Assign(value); 
	return 0; // success
}
//HotKeyIt ahkExecuteLine()
EXPORT unsigned int ahkExecuteLine(unsigned int line,unsigned int aMode,unsigned int wait)
{
	Line *templine = (Line *)line;
	if (templine == NULL)
		return (unsigned int)g_script.mFirstLine;
	else if (aMode && templine->mPrevLine != NULL)
	{
		if (wait)
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)templine, (LPARAM)aMode);
		else
			PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)templine, (LPARAM)aMode);
	}
	return (unsigned int) templine->mNextLine;
}

EXPORT unsigned int ahkLabel(LPTSTR aLabelName, unsigned int wait)
{
	Label *aLabel = g_script.FindLabel(aLabelName) ;
	if (aLabel)
	{
		if (wait)
			SendMessage(g_hWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		else
			PostMessage(g_hWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		return 1;
	}
	else
		return 0;
}

EXPORT unsigned int ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10)
{
	Func *aFunc = g_script.FindFunc(func) ;
	if (aFunc)
	{	
		g_script.mTempFunc = aFunc ;
		ExprTokenType return_value;
		if (aFunc->mParamCount > 0 && param1 != NULL)
		{
			// Copy the appropriate values into each of the function's formal parameters.
			aFunc->mParam[0].var->Assign((LPTSTR )param1); // Assign parameter #1
			if (aFunc->mParamCount > 1  && param2 != NULL) // Assign parameter #2
			{
				// v1.0.38.01: LPARAM is now written out as a DWORD because the majority of system messages
				// use LPARAM as a pointer or other unsigned value.  This shouldn't affect most scripts because
				// of the way ATOI64() and ATOU() wrap a negative number back into the unsigned domain for
				// commands such as PostMessage/SendMessage.
				aFunc->mParam[1].var->Assign((LPTSTR )param2);
				if (aFunc->mParamCount > 2 && param3 != NULL) // Assign parameter #3
				{
					aFunc->mParam[2].var->Assign((LPTSTR )param3);
					if (aFunc->mParamCount > 3 && param4 != NULL) // Assign parameter #4
					{
						aFunc->mParam[3].var->Assign((LPTSTR )param4);
						if (aFunc->mParamCount > 4 && param5 != NULL) // Assign parameter #5
						{
							aFunc->mParam[4].var->Assign((LPTSTR )param5);
							if (aFunc->mParamCount > 5 && param6 != NULL) // Assign parameter #6
							{
								aFunc->mParam[5].var->Assign((LPTSTR )param6);
								if (aFunc->mParamCount > 6 && param7 != NULL) // Assign parameter #7
								{
									aFunc->mParam[6].var->Assign((LPTSTR )param7);
									if (aFunc->mParamCount > 7 && param8 != NULL) // Assign parameter #8
									{
										aFunc->mParam[7].var->Assign((LPTSTR )param8);
										if (aFunc->mParamCount > 8 && param9 != NULL) // Assign parameter #9
										{
											aFunc->mParam[8].var->Assign((LPTSTR )param9);
											if (aFunc->mParamCount > 9 && param10 != NULL) // Assign parameter #10
											{
												aFunc->mParam[9].var->Assign((LPTSTR )param10);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		PostMessage(g_hWnd, AHK_EXECUTE_FUNCTION_DLL, (WPARAM)&return_value,NULL);
		return 0;
	}
	return -1;
}


EXPORT int ahkKey(LPTSTR keys) 
{
SendKeys(keys, false, SM_EVENT, 0, 1); // N11 sendahk
return 0;
}

#ifndef AUTOHOTKEYSC
#ifdef USRDLL
// Naveen: v6 addFile()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure)
{   // dynamically include a file into a script !!
	// labels, hotkeys, functions.   
	static int filesAdded = 0  ; 
	
	Line *oldLastLine = g_script.mLastLine;
	
	if (aIgnoreLoadFailure > 1)  // if third param is > 1, reset all functions, labels, remove hotkeys
	{
		g_script.mFuncCount = 0;   
		g_script.mFirstLabel = NULL ; 
		g_script.mLastLabel = NULL ; 
		g_script.mLastFunc = NULL ; 
	    g_script.mFirstLine = NULL ; 
		g_script.mLastLine = NULL ;
		 g_script.mCurrLine = NULL ; 

		if (filesAdded == 0)
			{
			SimpleHeap::sBlockCount = 0 ;
			SimpleHeap::sFirst = NULL;
			SimpleHeap::sLast  = NULL;
			SimpleHeap::sMostRecentlyAllocated = NULL;
			}
		if (filesAdded > 0)
			{
			// Naveen v9 free simpleheap memory for late include files
			SimpleHeap *next, *curr;
			for (curr = SimpleHeap::sFirst; curr != NULL;)
				{
				next = curr->mNextBlock;  // Save this member's value prior to deleting the object.
				curr->~SimpleHeap() ;
				curr = next;
				}
			SimpleHeap::sBlockCount = 0 ;
			SimpleHeap::sFirst = NULL;
			SimpleHeap::sLast  = NULL;
			SimpleHeap::sMostRecentlyAllocated = NULL;
/*  Naveen: the following is causing a memory leak in the exe version of clearing the simple heap v10
 g_script.mVar = NULL ; 
 g_script.mVarCount = 0 ; 
 g_script.mVarCountMax = 0 ; 
 g_script.mLazyVar = NULL ; 

 g_script.mLazyVarCount = 0 ; 
*/
		}
	g_script.LoadIncludedFile(fileName, aAllowDuplicateInclude, (bool) aIgnoreLoadFailure);
	g_script.PreparseBlocks(g_script.mFirstLine); 
	PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script.mFirstLine, (LPARAM)g_script.mFirstLine);
	filesAdded += 1;
	return (unsigned int) g_script.mFirstLine;
	}
	else
	{
	g_script.LoadIncludedFile(fileName, aAllowDuplicateInclude, (bool) aIgnoreLoadFailure);
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	return (unsigned int) oldLastLine->mNextLine;  // 
	}
return 0;  // never reached
}

#else
// Naveen: v6 addFile()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure)
{   // dynamically include a file into a script !!
	// labels, hotkeys, functions.   
	
	Line *oldLastLine = g_script.mLastLine;
	
	if (aIgnoreLoadFailure > 1)  // if third param is > 1, reset all functions, labels, remove hotkeys
	{
		g_script.mFuncCount = 0;   
		g_script.mFirstLabel = NULL ; 
		g_script.mLastLabel = NULL ; 
		g_script.mLastFunc = NULL ; 
		g_script.LoadIncludedFile(fileName, aAllowDuplicateInclude, aIgnoreLoadFailure);
	}
	else 
	{
		g_script.LoadIncludedFile(fileName, aAllowDuplicateInclude, (bool) aIgnoreLoadFailure);
	}
	
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	return (unsigned int) oldLastLine->mNextLine;  // 
}


#endif // USRDLL
#endif // AUTOHOTKEYSC

#ifndef AUTOHOTKEYSC
#ifdef USRDLL
// HotKeyIt: addScript()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addScript(LPTSTR script, int aExecute)
{   // dynamically include a script into a script !!
	// labels, hotkeys, functions.   

	Line *oldLastLine = g_script.mLastLine;
	
	g_script.LoadIncludedText(script);
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	if (aExecute > 0)
	{
		if (aExecute > 1)
		{
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)oldLastLine->mNextLine, (LPARAM)oldLastLine->mNextLine);
		}
		else
			PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)oldLastLine->mNextLine, (LPARAM)oldLastLine->mNextLine);
	}
	return (unsigned int) oldLastLine->mNextLine;  // 
}

#else
// HotKeyIt: addScript()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addScript(LPTSTR script, int aExecute)
{   // dynamically include a script from text!!
	// labels, hotkeys, functions.   
	
	Line *oldLastLine = g_script.mLastLine;

	g_script.LoadIncludedText(script);
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	if (aExecute > 0)
	{
		if (aExecute > 1)
			SendMessage(g_hWnd, AHK_EXECUTE, (WPARAM)oldLastLine->mNextLine, (LPARAM)oldLastLine->mNextLine);
		else
			PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)oldLastLine->mNextLine, (LPARAM)oldLastLine->mNextLine);
	}
	return (unsigned int) oldLastLine->mNextLine;  // 
}
#endif // USRDLL
#endif // AUTOHOTKEYSC
// HotKeyIt  -  ahkFunction can return a value now
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10)
{
	Func *aFunc = g_script.FindFunc(func) ;
	if (aFunc)
	{	
		g_script.mTempFunc = aFunc ;
		ExprTokenType return_value;
		if (aFunc->mParamCount > 0 && param1 != NULL)
		{
			// Copy the appropriate values into each of the function's formal parameters.
			aFunc->mParam[0].var->Assign((LPTSTR )param1); // Assign parameter #1
			if (aFunc->mParamCount > 1  && param2 != NULL) // Assign parameter #2
			{
				// v1.0.38.01: LPARAM is now written out as a DWORD because the majority of system messages
				// use LPARAM as a pointer or other unsigned value.  This shouldn't affect most scripts because
				// of the way ATOI64() and ATOU() wrap a negative number back into the unsigned domain for
				// commands such as PostMessage/SendMessage.
				aFunc->mParam[1].var->Assign((LPTSTR )param2);
				if (aFunc->mParamCount > 2 && param3 != NULL) // Assign parameter #3
				{
					aFunc->mParam[2].var->Assign((LPTSTR )param3);
					if (aFunc->mParamCount > 3 && param4 != NULL) // Assign parameter #4
					{
						aFunc->mParam[3].var->Assign((LPTSTR )param4);
						if (aFunc->mParamCount > 4 && param5 != NULL) // Assign parameter #5
						{
							aFunc->mParam[4].var->Assign((LPTSTR )param5);
							if (aFunc->mParamCount > 5 && param6 != NULL) // Assign parameter #6
							{
								aFunc->mParam[5].var->Assign((LPTSTR )param6);
								if (aFunc->mParamCount > 6 && param7 != NULL) // Assign parameter #7
								{
									aFunc->mParam[6].var->Assign((LPTSTR )param7);
									if (aFunc->mParamCount > 7 && param8 != NULL) // Assign parameter #8
									{
										aFunc->mParam[7].var->Assign((LPTSTR )param8);
										if (aFunc->mParamCount > 8 && param9 != NULL) // Assign parameter #9
										{
											aFunc->mParam[8].var->Assign((LPTSTR )param9);
											if (aFunc->mParamCount > 9 && param10 != NULL) // Assign parameter #10
											{
												aFunc->mParam[9].var->Assign((LPTSTR )param10);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		SendMessage(g_hWnd, AHK_EXECUTE_FUNCTION_DLL, (WPARAM)&return_value,NULL);
		return result_to_return_dll;
	}
	else
		return _T(""); 
}

bool callFuncDll()
{
	Func &func = *(Func *)g_script.mTempFunc ;
	if (!INTERRUPTIBLE_IN_EMERGENCY)
		return false;
	if (g_nThreads >= g_MaxThreadsTotal)
		// Below: Only a subset of ACT_IS_ALWAYS_ALLOWED is done here because:
		// 1) The omitted action types seem too obscure to grant always-run permission for msg-monitor events.
		// 2) Reduction in code size.
		if (g_nThreads >= MAX_THREADS_EMERGENCY // To avoid array overflow, this limit must by obeyed except where otherwise documented.
			|| func.mJumpToLine->mActionType != ACT_EXITAPP && func.mJumpToLine->mActionType != ACT_RELOAD)
			return false;

	// Need to check if backup is needed in case script explicitly called the function rather than using
	// it solely as a callback.  UPDATE: And now that max_instances is supported, also need it for that.
	// See ExpandExpression() for detailed comments about the following section.
	VarBkp *var_backup = NULL;   // If needed, it will hold an array of VarBkp objects.
	int var_backup_count; // The number of items in the above array.
	if (func.mInstances > 0) // Backup is needed.
		if (!Var::BackupFunctionVars(func, var_backup, var_backup_count)) // Out of memory.
			return false;
			// Since we're in the middle of processing messages, and since out-of-memory is so rare,
			// it seems justifiable not to have any error reporting and instead just avoid launching
			// the new thread.

	// Since above didn't return, the launch of the new thread is now considered unavoidable.

	// See MsgSleep() for comments about the following section.
	TCHAR ErrorLevel_saved[ERRORLEVEL_SAVED_SIZE];
	tcslcpy(ErrorLevel_saved, g_ErrorLevel->Contents(), _countof(ErrorLevel_saved));
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);


	// v1.0.38.04: Below was added to maximize responsiveness to incoming messages.  The reasoning
	// is similar to why the same thing is done in MsgSleep() prior to its launch of a thread, so see
	// MsgSleep for more comments:
	g_script.mLastScriptRest = g_script.mLastPeekTime = GetTickCount();


		DEBUGGER_STACK_PUSH(SE_Thread, func.mJumpToLine, desc, func.mName)
	ExprTokenType aResultToken;
	func.Call(&aResultToken); // Call the UDF.
	
		DEBUGGER_STACK_POP()

	// Fix for v1.0.47: Must handle return_value BEFORE calling FreeAndRestoreFunctionVars() because return_value
	// might be the contents of one of the function's local variables (which are about to be free'd).
	if (aResultToken.symbol == PURE_INTEGER)
	{
		result_to_return_dll = (LPTSTR )realloc(result_to_return_dll,256);
		ITOA64(aResultToken.value_int64,result_to_return_dll);
	}
	else //if (return_value.symbol)
	{
		result_to_return_dll = (LPTSTR )realloc(result_to_return_dll,_tcslen(TokenToString(aResultToken))*2);
		_tcsncpy(result_to_return_dll,TokenToString(aResultToken),_tcslen(TokenToString(aResultToken))+1);
	}
	Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count);
	ResumeUnderlyingThread(ErrorLevel_saved);
	return 0 ;
}







bool callFunc(WPARAM awParam, LPARAM alParam)
{
	Func &func = *(Func *)g_script.mTempFunc ;   
	if (!INTERRUPTIBLE_IN_EMERGENCY)
		return false;

	if (g_nThreads >= g_MaxThreadsTotal)
		// Below: Only a subset of ACT_IS_ALWAYS_ALLOWED is done here because:
		// 1) The omitted action types seem too obscure to grant always-run permission for msg-monitor events.
		// 2) Reduction in code size.
		if (g_nThreads >= MAX_THREADS_EMERGENCY // To avoid array overflow, this limit must by obeyed except where otherwise documented.
			|| func.mJumpToLine->mActionType != ACT_EXITAPP && func.mJumpToLine->mActionType != ACT_RELOAD)
			return false;

	// Need to check if backup is needed in case script explicitly called the function rather than using
	// it solely as a callback.  UPDATE: And now that max_instances is supported, also need it for that.
	// See ExpandExpression() for detailed comments about the following section.
	VarBkp *var_backup = NULL;   // If needed, it will hold an array of VarBkp objects.
	int var_backup_count; // The number of items in the above array.
	if (func.mInstances > 0) // Backup is needed.
		if (!Var::BackupFunctionVars(func, var_backup, var_backup_count)) // Out of memory.
			return false;
			// Since we're in the middle of processing messages, and since out-of-memory is so rare,
			// it seems justifiable not to have any error reporting and instead just avoid launching
			// the new thread.

	// Since above didn't return, the launch of the new thread is now considered unavoidable.

	// See MsgSleep() for comments about the following section.
	TCHAR ErrorLevel_saved[ERRORLEVEL_SAVED_SIZE];
	tcslcpy(ErrorLevel_saved, g_ErrorLevel->Contents(), _countof(ErrorLevel_saved));
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);

	
	// See ExpandExpression() for detailed comments about the following section.
	if (func.mParamCount > 0)
	{
		// Copy the appropriate values into each of the function's formal parameters.
		func.mParam[0].var->Assign((LPTSTR )awParam); // Assign parameter #1: wParam
		if (func.mParamCount > 1) // Assign parameter #2: lParam
		{
			// v1.0.38.01: LPARAM is now written out as a DWORD because the majority of system messages
			// use LPARAM as a pointer or other unsigned value.  This shouldn't affect most scripts because
			// of the way ATOI64() and ATOU() wrap a negative number back into the unsigned domain for
			// commands such as PostMessage/SendMessage.
			func.mParam[1].var->Assign((LPTSTR )alParam);
		}
	}

	// v1.0.38.04: Below was added to maximize responsiveness to incoming messages.  The reasoning
	// is similar to why the same thing is done in MsgSleep() prior to its launch of a thread, so see
	// MsgSleep for more comments:
	g_script.mLastScriptRest = g_script.mLastPeekTime = GetTickCount();


		DEBUGGER_STACK_PUSH(SE_Thread, func.mJumpToLine, desc, func.mName)

	ExprTokenType return_value;
	func.Call(&return_value); // Call the UDF.
	
		DEBUGGER_STACK_POP()

	// Fix for v1.0.47: Must handle return_value BEFORE calling FreeAndRestoreFunctionVars() because return_value
	// might be the contents of one of the function's local variables (which are about to be free'd).
/*	bool block_further_processing = *return_value; // No need to check the following because they're implied for *return_value!=0: result != EARLY_EXIT && result != FAIL;
	if (block_further_processing)
		aMsgReply = (LPARAM)ATOI64(return_value); // Use 64-bit in case it's an unsigned number greater than 0x7FFFFFFF, in which case this allows it to wrap around to a negative.
	//else leave aMsgReply uninitialized because we'll be returning false later below, which tells our caller
	// to ignore aMsgReply.
*/
	Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count);
	ResumeUnderlyingThread(ErrorLevel_saved);
	
	return 0 ; // block_further_processing; // If false, the caller will ignore aMsgReply and process this message normally. If true, aMsgReply contains the reply the caller should immediately send for this message.
}
