
#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "exports.h"
#include "script.h"

// HotKeyIt check if dll is ready to execute
#ifdef DLLN
EXPORT BOOL EAhkReady()
{
	return g_script.mIsReadyToExecute;
}
#endif
// Naveen: v1. ahkgetvar()
EXPORT unsigned int ahkFindFunc(char *funcname)
{
return (unsigned int)g_script.FindFunc(funcname);
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
	aResultToken.marker = "";
	// Get the first arg, which is the string used as the source of the extraction. Call it "findfunc" for clarity.
	char funcname_buf[MAX_NUMBER_SIZE]; // A separate buf because aResultToken.buf is sometimes used to store the result.
	char *funcname = TokenToString(*aParam[0], funcname_buf); // Remember that aResultToken.buf is part of a union, though in this case there's no danger of overwriting it since our result will always be of STRING type (not int or float).
	int funcname_length = (int)EXPR_TOKEN_LENGTH(aParam[0], funcname);
	aResultToken.value_int64 = (__int64)ahkFindFunc(funcname);
	return;
}

EXPORT VarSizeType ahkgetvar(char *name, char *output)
{
	Var *ahkvar = g_script.FindOrAddVar(name);
	if ( ahkvar->mType == VAR_ALIAS )
		return ahkvar->mAliasFor->Get(output); //Hotkeyit removed ebiv.cpp and made ahkgetvar return all vars
 	else if ( ahkvar->mType == VAR_NORMAL )
		return ahkvar->Get(output);  // var.getText() added in V1.
	else if ( ahkvar->mType == 4 )
		return ahkvar->mBIV(output,name); //Hotkeyit
	return NULL;
}	

EXPORT int ahkassign(char *name, char *value) // ahkwine 0.1
{
	Var *var;
if (   !(var = g_script.FindOrAddVar(name, strlen(name)))   )
				return -1;  // Realistically should never happen.
			var->Assign(value); 
			return 0; // success
}

EXPORT int ahkLabel(char *aLabelName)
{
	Label *aLabel = g_script.FindLabel(aLabelName) ;
	if (aLabel)
	{
		PostMessage(g_hWnd, AHK_EXECUTE_LABEL, (LPARAM)aLabel, (LPARAM)aLabel);
		return 0;
	}
	else
		return -1;
}

EXPORT int ahkKey(char *keys) 
{
SendKeys(keys, false, SM_EVENT, 0, 1); // N11 sendahk
return 0;
}


#ifdef DLLN
// Naveen: v6 addFile()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addFile(char *fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure)
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
EXPORT unsigned int addFile(char *fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure)
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


#endif

#ifdef DLLN
// HotKeyIt: addScript()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addScript(char *script, int aReplace)
{   // dynamically include a script into a script !!
	// labels, hotkeys, functions.   
	static int filesAdded = 0  ; 
	
	Line *oldLastLine = g_script.mLastLine;
	
	if (aReplace > 0)  // if second param is > 1, reset all functions, labels, remove hotkeys
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
	g_script.LoadFromScript(script);
	g_script.PreparseBlocks(g_script.mFirstLine); 
	PostMessage(g_hWnd, AHK_EXECUTE, (WPARAM)g_script.mFirstLine, (LPARAM)g_script.mFirstLine);
	filesAdded += 1;
	return (unsigned int) g_script.mFirstLine;
	}
	else
	{
	g_script.LoadFromScript(script);
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	return (unsigned int) oldLastLine->mNextLine;  // 
	}
return 0;  // never reached
}

#else
// HotKeyIt: addScript()
// Todo: support for #Directives, and proper treatment of mIsReadytoExecute
EXPORT unsigned int addScript(char *script, int aReplace)
{   // dynamically include a script from text!!
	// labels, hotkeys, functions.   
	
	Line *oldLastLine = g_script.mLastLine;
	
	if (aReplace > 0)  // if third param is > 1, reset all functions, labels, remove hotkeys
	{
		g_script.mFuncCount = 0;   
		g_script.mFirstLabel = NULL ; 
		g_script.mLastLabel = NULL ; 
		g_script.mLastFunc = NULL ; 
		g_script.LoadFromScript(script);
	}
	else 
	{
		g_script.LoadFromScript(script);
	}
	
	g_script.PreparseBlocks(oldLastLine->mNextLine); // 
	return (unsigned int) oldLastLine->mNextLine;  // 
}


#endif
void BIF_Import(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount) // Added in Nv8.
{
	// Set default return value in case of early return.
	aResultToken.symbol = SYM_INTEGER ;
	aResultToken.marker = "";
	bool aIgnoreLoadFailure = false ;
	bool aAllowDuplicateInclude = false ;
	// Get the first arg, which is the string used as the source of the extraction. Call it "haystack" for clarity.
	char haystack_buf[MAX_NUMBER_SIZE]; // A separate buf because aResultToken.buf is sometimes used to store the result.
	char *haystack = TokenToString(*aParam[0], haystack_buf); // Remember that aResultToken.buf is part of a union, though in this case there's no danger of overwriting it since our result will always be of STRING type (not int or float).
	int haystack_length = (int)EXPR_TOKEN_LENGTH(aParam[0], haystack);

	if (aParamCount < 2)// Load-time validation has ensured that at least the first parameter is present:
	{
		aResultToken.value_int64 = (__int64)addFile(haystack, false, 0);
		//  Hotkey::HookUp() ; didn't work: see if we can remove dependence on having to suspend * 2 to enable hotkeys Nv8.
		return;
	}
	else	
		aAllowDuplicateInclude = (bool)TokenToInt64(*aParam[1]); // The one-based starting position in haystack (if any).  Convert it to zero-based.
		__int64 clear = TokenToInt64(*aParam[2]) ;
#ifndef AUTOHOTKEYSC		
	aResultToken.value_int64 = (__int64)addFile(haystack, aAllowDuplicateInclude, (int)clear);
#endif
	return;
}

// HotKeyIt  -  ahkFunction can return a value now
EXPORT void* ahkFunction(char *func, char *param1, char *param2, char *param3, char *param4, char *param5, char *param6, char *param7, char *param8, char *param9, char *param10)
{
	Func *aFunc = g_script.FindFunc(func) ;
if (aFunc)
{	
	static char *result_to_return;
	Func &func = *(Func *)aFunc;//g_script.mTempFunc ;   
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
	char ErrorLevel_saved[ERRORLEVEL_SAVED_SIZE];

	strlcpy(ErrorLevel_saved, g_ErrorLevel->Contents(), sizeof(ErrorLevel_saved));
	
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);
	++g_script.mTimerEnabledCount;
	
	// See ExpandExpression() for detailed comments about the following section.
	if (func.mParamCount > 0)
	{
		// Copy the appropriate values into each of the function's formal parameters.
		func.mParam[0].var->Assign((char *)param1); // Assign parameter #1
		if (func.mParamCount > 1) // Assign parameter #2
		{
			// v1.0.38.01: LPARAM is now written out as a DWORD because the majority of system messages
			// use LPARAM as a pointer or other unsigned value.  This shouldn't affect most scripts because
			// of the way ATOI64() and ATOU() wrap a negative number back into the unsigned domain for
			// commands such as PostMessage/SendMessage.
			func.mParam[1].var->Assign((char *)param2);
			if (func.mParamCount > 2) // Assign parameter #3
			{
				func.mParam[2].var->Assign((char *)param3);
				if (func.mParamCount > 3) // Assign parameter #4
				{
					func.mParam[3].var->Assign((char *)param4);
					if (func.mParamCount > 4) // Assign parameter #5
					{
						func.mParam[4].var->Assign((char *)param5);
						if (func.mParamCount > 5) // Assign parameter #6
						{
							func.mParam[5].var->Assign((char *)param6);
							if (func.mParamCount > 6) // Assign parameter #7
							{
								func.mParam[6].var->Assign((char *)param7);
							if (func.mParamCount > 7) // Assign parameter #8
							{
								func.mParam[7].var->Assign((char *)param8);
								if (func.mParamCount > 8) // Assign parameter #9
								{
									func.mParam[8].var->Assign((char *)param9);
									if (func.mParamCount > 9) // Assign parameter #10
									{
										func.mParam[9].var->Assign((char *)param10);
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

	// v1.0.38.04: Below was added to maximize responsiveness to incoming messages.  The reasoning
	// is similar to why the same thing is done in MsgSleep() prior to its launch of a thread, so see
	// MsgSleep for more comments:
	g_script.mLastScriptRest = g_script.mLastPeekTime = GetTickCount();


		DEBUGGER_STACK_PUSH(SE_Thread, func.mJumpToLine, desc, func.mName)

	ExprTokenType return_value;
	ResultType result;
	g_DeferMessagesForUnderlyingPump = true;
	result = func.Call(&return_value); // Call the UDF.
	g_DeferMessagesForUnderlyingPump = false;
	--g_script.mTimerEnabledCount;
		DEBUGGER_STACK_POP()
	if (return_value.symbol == PURE_INTEGER)
	{
		char buf[256];
		result_to_return = buf;
		ITOA64(return_value.value_int64,result_to_return);
 		Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count);
		ResumeUnderlyingThread(ErrorLevel_saved);
		return result_to_return;
	}
	else //if (return_value.symbol)
	{
		result_to_return = (char *)realloc(result_to_return,strlen(TokenToString(return_value))+1);
		strncpy(result_to_return,TokenToString(return_value),strlen(TokenToString(return_value))+1);
 		Var::FreeAndRestoreFunctionVars(func, var_backup, var_backup_count);
		ResumeUnderlyingThread(ErrorLevel_saved);
		return result_to_return;
	}
}
else
	return "-1"; 
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
	char ErrorLevel_saved[ERRORLEVEL_SAVED_SIZE];
	strlcpy(ErrorLevel_saved, g_ErrorLevel->Contents(), sizeof(ErrorLevel_saved));
	InitNewThread(0, false, true, func.mJumpToLine->mActionType);

	
	// See ExpandExpression() for detailed comments about the following section.
	if (func.mParamCount > 0)
	{
		// Copy the appropriate values into each of the function's formal parameters.
		func.mParam[0].var->Assign((char *)awParam); // Assign parameter #1: wParam
		if (func.mParamCount > 1) // Assign parameter #2: lParam
		{
			// v1.0.38.01: LPARAM is now written out as a DWORD because the majority of system messages
			// use LPARAM as a pointer or other unsigned value.  This shouldn't affect most scripts because
			// of the way ATOI64() and ATOU() wrap a negative number back into the unsigned domain for
			// commands such as PostMessage/SendMessage.
			func.mParam[1].var->Assign((char *)alParam);
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






int initPlugins() // N10 plugins
{
	g_script.AddBIF("Import", BIF_Import, 1, 3) ;

	g_script.xifwinactive = NULL ;
    g_script.xwingetid = NULL ;
    g_script.xsend = NULL ;
	
	return 0 ;
}

EXPORT int ImportBIF(char *name, BuiltInFunctionType bif, size_t min, size_t max) //N10 plugins
{
return g_script.AddBIF(name, bif, min, max);
}
