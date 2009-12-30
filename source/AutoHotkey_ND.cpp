/*
AutoHotkey

Copyright 2003-2009 Chris Mallett (support@autohotkey.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifdef DLLN
#include "stdafx.h" // pre-compiled headers
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "window.h" // For MsgBox() & SetForegroundLockTimeout()
#include "exports.h"

// General note:
// The use of Sleep() should be avoided *anywhere* in the code.  Instead, call MsgSleep().
// The reason for this is that if the keyboard or mouse hook is installed, a straight call
// to Sleep() will cause user keystrokes & mouse events to lag because the message pump
// (GetMessage() or PeekMessage()) is the only means by which events are ever sent to the
// hook functions.

// Naveen: v1. #Include process.h for begin threadx 
#include <process.h>  

// Naveen: v3. struct nameHinstance 
// carries startup paramaters for script
// Todo: change name to something more intuitive
static struct nameHinstance
     {
       HINSTANCE hInstanceP;
       char *name;
	   char argv[1000];
	   char args[1000];
	   int istext;
     } nameHinstanceP ;

// Naveen v1. HANDLE hThread
// Todo: move this to struct nameHinstance
static 	HANDLE hThread;

// Naveen v1. hThread2 and threadCount
// Todo: remove these as multithreading was implemented 
//       with multiple loading of the dll under separate names.
static int threadCount = 1 ; 
static 	HANDLE hThread2;

// Naveen v1. DllMain() - puts hInstance into struct nameHinstanceP 
//                        so it can be passed to OldWinMain()
// hInstance is required for script initialization 
// probably for window creation
// Todo: better cleanup in DLL_PROCESS_DETACH: windows, variables, no exit from script

BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD fwdReason, LPVOID lpvReserved)
 {
switch(fwdReason)
 {
 case DLL_PROCESS_ATTACH:
	 {
	nameHinstanceP.hInstanceP = hInstance;
#ifdef AUTODLL
	ahkdll("autoload.ahk", "", "");	  // used for remoteinjection of dll 
#endif
	   break;
	 }
 case DLL_THREAD_ATTACH:

 break;

 case DLL_PROCESS_DETACH:
	 {
		 CloseHandle( hThread ); // need better cleanup: windows, variables, no exit from script
		 break;
	 }
 case DLL_THREAD_DETACH:
 break;
 }

 return(TRUE); // a FALSE will abort the DLL attach
 }





// int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 

// Naveen v1. changed WinMain() to OldWinMain()
//            ahkdll() will call this through runscript() after DllMain()
// Todo: separate AutoHotkey.exe_N project with a WinMain() to use the AutoHotkey.dll library

int WINAPI OldWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	setvbuf(stdout, NULL, _IONBF, 0); // L17: Disable stdout buffering to make it a more effective debugging tool.
	
	// Init any globals not in "struct g" that need it:
	g_hInstance = hInstance;
	g_persistent = true;
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).

	if (!GetCurrentDirectory(sizeof(g_WorkingDir), g_WorkingDir)) // Needed for the FileSelectFile() workaround.
		*g_WorkingDir = '\0';
	// Unlike the below, the above must not be Malloc'd because the contents can later change to something
	// as large as MAX_PATH by means of the SetWorkingDir command.
	g_WorkingDirOrig = SimpleHeap::Malloc(g_WorkingDir); // Needed by the Reload command.
	
	// Set defaults, to be overridden by command line args we receive:
	bool restart_mode = false;
	
#ifndef AUTOHOTKEYSC
	char *script_filespec = lpCmdLine ; // Naveen changed from NULL;
#endif


	char var_name[32], *param; // Small size since only numbers will be used (e.g. %1%, %2%).
	Var *var;
	bool switch_processing_is_complete = false;
	int script_param_num = 1;

	
/*  
//             Naveen: v3. replaced command line __argc and __argv above with args[]
//             currently only a single parameter such as /Debug is allowed in args[1]
//             a single script parameter may contain white space in args[0]
//             the client dll can use StringSplit to extract individual parameters
*/

char *args[4];
args[0] = __argv[0];   //      name of host program
args[1] = nameHinstanceP.argv;  // 1 option such as /Debug  /R /F /STDOUT
args[2] = nameHinstanceP.name;  // name of script to launch
args[3] = nameHinstanceP.args;  // script parameters, all in one string (* char)
int argc = 4;

	for (int i = 1; i < argc; ++i)  //	Naveen changed from:  for (int i = 1; i < __argc; ++i) see above
	{  // Naveen: v6.1 put options in script variables as well
		param = args[i]; // Naveen changed from: __argv[i]; see above
			if (   !(var = g_script.FindOrAddVar(var_name, sprintf(var_name, "%d", script_param_num)))   )
				return CRITICAL_ERROR;  // Realistically should never happen.
			var->Assign(param);
			++script_param_num;
		
	}   // Naveen: v6.1 only argv needs special processing
	    //              script will do its own parameter parsing

param = nameHinstanceP.argv ; // Naveen: v6.1 Script options in nameHinstanceP.name will be processed here
// Naveen: v6.1 more relaxed parsing of script options
// using strcasestr instead of stricmp, too lazy to use va_arg for now.  
/*		 if (!strcasestr(param, "/R") || !strcasestr(param, "/restart"))
		    restart_mode = true;
		 if (!strcasestr(param, "/F") || !strcasestr(param, "/force"))
			g_ForceLaunch = true;

			if (!strcasestr(param, "/ErrorStdOut"))
			g_script.mErrorStdOut = true;
*/
/*   // Naveen: v6.1 removed __argv parsing one by one
		// Insist that switches be an exact match for the allowed values to cut down on ambiguity.
		// For example, if the user runs "CompiledScript.exe /find", we want /find to be considered
		// an input parameter for the script rather than a switch:
			
		 else if (!stricmp(param, "/R") || !stricmp(param, "/restart"))
			restart_mode = true;
		 else if (!stricmp(param, "/F") || !stricmp(param, "/force"))
			g_ForceLaunch = true;
		 else if (!stricmp(param, "/ErrorStdOut"))
			g_script.mErrorStdOut = true;

			// Naveen: v6.1 removed option /iLib, for now...
#ifndef AUTOHOTKEYSC // i.e. the following switch is recognized only by AutoHotkey.exe (especially since recognizing new switches in compiled scripts can break them, unlike AutoHotkey.exe).
		 else if (!stricmp(param, "/iLib")) // v1.0.47: Build an include-file so that ahk2exe can include library functions called by the script.
		{
			++i; // Consume the next parameter too, because it's associated with this one.
			if (i >= argc) // Naveen: v6.1 changed from __argc // Missing the expected filename parameter.
				return CRITICAL_ERROR;
			// For performance and simplicity, open/crease the file unconditionally and keep it open until exit.
			if (   !(g_script.mIncludeLibraryFunctionsThenExit = fopen(__argv[i], "w"))   ) // Can't open the temp file.
				return CRITICAL_ERROR;
		}
#endif

*/
#ifdef SCRIPT_DEBUG
		// Allow a debug session to be initiated by command-line.
		 // Naveen: v6.1 changed stincmp to strcasetr

if (param = strcasestr(param, "/Debug"))
{
	                // Naveen TODO: build AutoHotkey.exe statically linked to AutoHotkey.dll
	// so I can debug in msvc.
		if (!g_Debugger.IsConnected()) //  && (param[6] == '\0' || param[6] == '=')
		{
			if (param[6] == '=')
			{
				param += 7;

				char *c = strrchr(param, ':');

				if (c)
				{
					g_DebuggerHost = SimpleHeap::Malloc(param, c-param);
					g_DebuggerHost[c-param] = '\0';
					g_DebuggerPort = SimpleHeap::Malloc(c + 1);
				}
				else
				{
					g_DebuggerHost = SimpleHeap::Malloc(param);
					g_DebuggerPort = "9000";
				}
			}
			else
			{
				g_DebuggerHost = "localhost";
				g_DebuggerPort = "9000";
			}
			// The actual debug session is initiated after the script is successfully parsed.
		}
}
#endif
/*  //Naveen: v6.1 removed parsing of script parameters... ahk script can do this itself.
else // since this is not a recognized switch, the end of the [Switches] section has been reached (by design).
		{
			switch_processing_is_complete = true;  // No more switches allowed after this point.
#ifdef AUTOHOTKEYSC
			--i; // Make the loop process this item again so that it will be treated as a script param.
#else
	//		script_filespec =	param;  // Naveen removed.  script_filespec is a separate parameter to OldWinMain() now.
#endif
		}
// Naveen v6.1 :end script option parsing
*/

#ifndef AUTOHOTKEYSC
	if (script_filespec)// Script filename was explicitly specified, so check if it has the special conversion flag.
	{
		size_t filespec_length = strlen(script_filespec);
		if (filespec_length >= CONVERSION_FLAG_LENGTH)
		{
			char *cp = script_filespec + filespec_length - CONVERSION_FLAG_LENGTH;
			// Now cp points to the first dot in the CONVERSION_FLAG of script_filespec (if it has one).
			if (!stricmp(cp, CONVERSION_FLAG))
				return Line::ConvertEscapeChar(script_filespec);
		}
	}
#endif

	// Like AutoIt2, store the number of script parameters in the script variable %0%, even if it's zero:
	if (   !(var = g_script.FindOrAddVar("0"))   )
		return CRITICAL_ERROR;  // Realistically should never happen.
	var->Assign(script_param_num - 1);

	// Naveen v6.1 added script vars: A_ScriptParams and A_ScriptOptions
	// 
	Var *A_ScriptParams;
	A_ScriptParams = g_script.FindOrAddVar("A_ScriptParams");	
	A_ScriptParams->Assign(nameHinstanceP.args);

	Var *A_ScriptOptions;
	A_ScriptOptions = g_script.FindOrAddVar("A_ScriptOptions");	
	A_ScriptOptions->Assign(nameHinstanceP.argv);

// Naveen Todo: change 'g' to a more descriptive and easily searchable name such as threadStruct
	global_init(*g);  // Set defaults prior to the below, since below might override them for AutoIt2 scripts.

	initPlugins(); // N10 plugins
// Set up the basics of the script:
#ifdef AUTOHOTKEYSC
	if (g_script.Init(*g, "", restart_mode) != OK) 
		return CRITICAL_ERROR;
#else //HotKeyIt: Go different init routine when file does not exist and assume this is a script to load
	if (!nameHinstanceP.istext)
	{
		if (g_script.Init(*g, script_filespec, restart_mode) != OK)  // Set up the basics of the script, using the above.
			return CRITICAL_ERROR;
	}
	else
	{
		if (g_script.InitDll(*g,hInstance) != OK)  // Set up the basics of the script.
			return CRITICAL_ERROR;
	}
#endif

	// Set g_default now, reflecting any changes made to "g" above, in case AutoExecSection(), below,
	// never returns, perhaps because it contains an infinite loop (intentional or not):
	CopyMemory(&g_default, g, sizeof(global_struct));

	// Could use CreateMutex() but that seems pointless because we have to discover the
	// hWnd of the existing process so that we can close or restart it, so we would have
	// to do this check anyway, which serves both purposes.  Alt method is this:
	// Even if a 2nd instance is run with the /force switch and then a 3rd instance
	// is run without it, that 3rd instance should still be blocked because the
	// second created a 2nd handle to the mutex that won't be closed until the 2nd
	// instance terminates, so it should work ok:
	//CreateMutex(NULL, FALSE, script_filespec); // script_filespec seems a good choice for uniqueness.
	//if (!g_ForceLaunch && !restart_mode && GetLastError() == ERROR_ALREADY_EXISTS)

#ifdef AUTOHOTKEYSC
	LineNumberType load_result = g_script.LoadFromFile();
#else //HotKeyIt changed to load from Text in dll as well when file does not exist
	LineNumberType load_result = !nameHinstanceP.istext ? g_script.LoadFromFile(script_filespec == NULL) : g_script.LoadText(script_filespec);
#endif
	if (load_result == LOADING_FAILED) // Error during load (was already displayed by the function call).
		return CRITICAL_ERROR;  // Should return this value because PostQuitMessage() also uses it.
	if (!load_result) // LoadFromFile() relies upon us to do this check.  No lines were loaded, so we're done.
		return 0;

	// Unless explicitly set to be non-SingleInstance via SINGLE_INSTANCE_OFF or a special kind of
	// SingleInstance such as SINGLE_INSTANCE_REPLACE and SINGLE_INSTANCE_IGNORE, persistent scripts
	// and those that contain hotkeys/hotstrings are automatically SINGLE_INSTANCE_PROMPT as of v1.0.16:
	if (g_AllowOnlyOneInstance == ALLOW_MULTI_INSTANCE && IS_PERSISTENT)
		g_AllowOnlyOneInstance = SINGLE_INSTANCE_PROMPT;

	HWND w_existing = NULL;
	UserMessages reason_to_close_prior = (UserMessages)0;
	if (g_AllowOnlyOneInstance && g_AllowOnlyOneInstance != SINGLE_INSTANCE_OFF && !restart_mode && !g_ForceLaunch)
	{
		// Note: the title below must be constructed the same was as is done by our
		// CreateWindows(), which is why it's standardized in g_script.mMainWindowTitle:
		if (w_existing = FindWindow(WINDOW_CLASS_MAIN, g_script.mMainWindowTitle))
		{
			if (g_AllowOnlyOneInstance == SINGLE_INSTANCE_IGNORE)
				return 0;
			if (g_AllowOnlyOneInstance != SINGLE_INSTANCE_REPLACE)
				if (MsgBox("An older instance of this script is already running.  Replace it with this"
					" instance?\nNote: To avoid this message, see #SingleInstance in the help file."
					, MB_YESNO, g_script.mFileName) == IDNO)
					return 0;
			// Otherwise:
			reason_to_close_prior = AHK_EXIT_BY_SINGLEINSTANCE;
		}
	}
	if (!reason_to_close_prior && restart_mode)
		if (w_existing = FindWindow(WINDOW_CLASS_MAIN, g_script.mMainWindowTitle))
			reason_to_close_prior = AHK_EXIT_BY_RELOAD;
	if (reason_to_close_prior)
	{
		// Now that the script has been validated and is ready to run, close the prior instance.
		// We wait until now to do this so that the prior instance's "restart" hotkey will still
		// be available to use again after the user has fixed the script.  UPDATE: We now inform
		// the prior instance of why it is being asked to close so that it can make that reason
		// available to the OnExit subroutine via a built-in variable:
		ASK_INSTANCE_TO_CLOSE(w_existing, reason_to_close_prior);
		//PostMessage(w_existing, WM_CLOSE, 0, 0);

		// Wait for it to close before we continue, so that it will deinstall any
		// hooks and unregister any hotkeys it has:
		int interval_count;
		for (interval_count = 0; ; ++interval_count)
		{
			Sleep(20);  // No need to use MsgSleep() in this case.
			if (!IsWindow(w_existing))
				break;  // done waiting.
			if (interval_count == 100)
			{
				// This can happen if the previous instance has an OnExit subroutine that takes a long
				// time to finish, or if it's waiting for a network drive to timeout or some other
				// operation in which it's thread is occupied.
				if (MsgBox("Could not close the previous instance of this script.  Keep waiting?", 4) == IDNO)
					return CRITICAL_ERROR;
				interval_count = 0;
			}
		}
		// Give it a small amount of additional time to completely terminate, even though
		// its main window has already been destroyed:
		Sleep(100);
	}

	// Call this only after closing any existing instance of the program,
	// because otherwise the change to the "focus stealing" setting would never be undone:
	SetForegroundLockTimeout();

	// Create all our windows and the tray icon.  This is done after all other chances
	// to return early due to an error have passed, above.
	if (g_script.CreateWindows() != OK)
		return CRITICAL_ERROR;

	// At this point, it is nearly certain that the script will be executed.

	if (g_MaxHistoryKeys && (g_KeyHistory = (KeyHistoryItem *)malloc(g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
		ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
	//else leave it NULL as it was initialized in globaldata.

	// MSDN: "Windows XP: If a manifest is used, InitCommonControlsEx is not required."
	// Therefore, in case it's a high overhead call, it's not done on XP or later:
	if (!g_os.IsWinXPorLater())
	{
		// Since InitCommonControls() is apparently incapable of initializing DateTime and MonthCal
		// controls, InitCommonControlsEx() must be called.  But since Ex() requires comctl32.dll
		// 4.70+, must get the function's address dynamically in case the program is running on
		// Windows 95/NT without the updated DLL (otherwise the program would not launch at all).
		typedef BOOL (WINAPI *MyInitCommonControlsExType)(LPINITCOMMONCONTROLSEX);
		MyInitCommonControlsExType MyInitCommonControlsEx = (MyInitCommonControlsExType)
			GetProcAddress(GetModuleHandle("comctl32"), "InitCommonControlsEx"); // LoadLibrary shouldn't be necessary because comctl32 in linked by compiler.
		if (MyInitCommonControlsEx)
		{
			INITCOMMONCONTROLSEX icce;
			icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
			icce.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES; // ICC_WIN95_CLASSES is equivalent to calling InitCommonControls().
			MyInitCommonControlsEx(&icce);
		}
		else // InitCommonControlsEx not available, so must revert to non-Ex() to make controls work on Win95/NT4.
			InitCommonControls();
	}

#ifdef SCRIPT_DEBUG
	// Initiate debug session now if applicable.
	if (g_DebuggerHost && g_Debugger.Connect(g_DebuggerHost, g_DebuggerPort) == DEBUGGER_E_OK)
	{
		g_Debugger.ProcessCommands();
	}
#endif

	// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
	// top part (the auto-execute part) of the script so that they will be in effect even if the
	// top part is something that's very involved and requires user interaction:
	Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
	Hotkey::InstallKeybdHook();
	if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
		AddRemoveHooks(3);
	g_script.mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
	
	//free(nameHinstanceP.name);

	Var *clipboard_var = g_script.FindOrAddVar("Clipboard"); // Add it if it doesn't exist, in case the script accesses "Clipboard" via a dynamic variable.
	if (clipboard_var)
		// This is done here rather than upon variable creation speed up runtime/dynamic variable creation.
		// Since the clipboard can be changed by activity outside the program, don't read-cache its contents.
		// Since other applications and the user should see any changes the program makes to the clipboard,
		// don't write-cache it either.
		clipboard_var->DisableCache();

	// Run the auto-execute part at the top of the script (this call might never return):
	if (!g_script.AutoExecSection()) // Can't run script at all. Due to rarity, just abort.
		return CRITICAL_ERROR;
	// REMEMBER: The call above will never return if one of the following happens:
	// 1) The AutoExec section never finishes (e.g. infinite loop).
	// 2) The AutoExec function uses uses the Exit or ExitApp command to terminate the script.
	// 3) The script isn't persistent and its last line is reached (in which case an ExitApp is implicit).

	// Call it in this special mode to kick off the main event loop.
	// Be sure to pass something >0 for the first param or it will
	// return (and we never want this to return):
	MsgSleep(SLEEP_INTERVAL, WAIT_FOR_MESSAGES);
	return 0; // Never executed; avoids compiler warning.
}

// Naveen: v1. runscript() - runs the script in a separate thread compared to host application.
unsigned __stdcall runScript( void* pArguments )
{
	struct nameHinstance a =  *(struct nameHinstance *)pArguments;
	HINSTANCE hInstance = a.hInstanceP;
	char *fileName = a.name;
	OldWinMain(hInstance, 0, fileName, 0);	
	_endthreadex( 0 );  
    return 0;
}


// Naveen: v1. ahkdll() - load AutoHotkey script into dll
// Naveen: v3. ahkdll(script, single command line option, script parameters)
// options such as /Debug are supported, see Lexikos' Debugger 
EXPORT unsigned int ahkdll(char *fileName, char *argv, char *args)
{
 unsigned threadID;
 // nameHinstanceP.name = fileName ;
 // nameHinstanceP.argv = argv ;
 // nameHinstanceP.args = args ;

 nameHinstanceP.name = (char *)realloc(nameHinstanceP.name,strlen(fileName)+1);
 strncpy(nameHinstanceP.name, fileName, strlen(fileName)+1);
 strncpy(nameHinstanceP.argv, argv, strlen(argv));
 strncpy(nameHinstanceP.args, args, strlen(args));

 hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, &threadID );
 DWORD lpExitCode = 0;
 while (!g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259))
 {
	 Sleep(50);
	 GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
 }
 return (unsigned int)hThread;
}

// HotKeyIt ahktextdll
EXPORT unsigned int ahktextdll(char *fileName, char *argv, char *args)
{
 unsigned threadID;
 nameHinstanceP.name = (char *)realloc(nameHinstanceP.name,strlen(fileName)+1);
 strncpy(nameHinstanceP.name, fileName, strlen(fileName)+1);
 strncpy(nameHinstanceP.argv, argv, strlen(argv));
 strncpy(nameHinstanceP.args, args, strlen(args));
 nameHinstanceP.istext = 1;

 hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, &threadID );
 int lpExitCode = 0;
 while (!g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259))
 {
	 Sleep(50);
	 GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
 }
 return (unsigned int)hThread;
}



EXPORT int ahkTerminate()
{
	TerminateThread(hThread, (DWORD)EARLY_RETURN);
	Line::sSourceFileCount = 0;
	g_script.Destroy();
	Line::sSourceFile = &nameHinstanceP.name;
	return 0;
}

EXPORT unsigned int ahkReload()
{
	unsigned threadID;
	Line::sSourceFileCount = 0;
	g_script.Destroy();
	HANDLE oldThread = hThread;
	hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, &threadID );
	TerminateThread(oldThread, (DWORD)EARLY_RETURN);
	return EARLY_RETURN;
}

EXPORT BOOL ahkReady() // HotKeyIt check if dll is ready to execute
{
	return g_script.mIsReadyToExecute;
}

/*

unsigned __stdcall ContinueScript( void* pArguments )
{
	MsgSleep(SLEEP_INTERVAL, WAIT_FOR_MESSAGES); ;
	_endthreadex( 0 );  
    return 0;
} 
EXPORT int ahkContinue()
{
	 unsigned threadID;
	hThread = (HANDLE)_beginthreadex( NULL, 0, &ContinueScript, &nameHinstanceP, 0, &threadID );
 return (int)hThread;
}

*/
#endif




