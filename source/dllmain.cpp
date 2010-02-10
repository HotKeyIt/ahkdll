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

#include "stdafx.h" // pre-compiled headers
#ifdef USRDLL
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "window.h" // For MsgBox() & SetForegroundLockTimeout()
#include "TextIO.h"

#include "windows.h"  // N11
#include "exports.h"  // N11
#include <process.h>  // N11
#include <string>

// General note:
// The use of Sleep() should be avoided *anywhere* in the code.  Instead, call MsgSleep().
// The reason for this is that if the keyboard or mouse hook is installed, a straight call
// to Sleep() will cause user keystrokes & mouse events to lag because the message pump
// (GetMessage() or PeekMessage()) is the only means by which events are ever sent to the
// hook functions.



static struct nameHinstance
     {
       HINSTANCE hInstanceP;
	   LPTSTR name ;
	   LPTSTR argv;
	   LPTSTR args;
	 //  TCHAR argv[1000];
	 //  TCHAR args[1000];
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

BOOL APIENTRY DllMain(HMODULE hInstance,DWORD fwdReason, LPVOID lpvReserved)
 {
switch(fwdReason)
 {
 case DLL_PROCESS_ATTACH:
	 {
		nameHinstanceP.hInstanceP = (HINSTANCE)hInstance;
#ifdef AUTODLL
	ahkdll("autoload.ahk", "", "");	  // used for remoteinjection of dll 
#endif
	   break;
	 }
 case DLL_THREAD_ATTACH:
	 {
		break;
	 }
 case DLL_PROCESS_DETACH:
	 {
		 int lpExitCode = 0;
		 GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
		 if ( lpExitCode == 259 )
			CloseHandle( hThread ); // need better cleanup: windows, variables, no exit from script
		 break;
	 }
 case DLL_THREAD_DETACH:
 break;
 }

 return(TRUE); // a FALSE will abort the DLL attach
 }


int WINAPI OldWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// Init any globals not in "struct g" that need it:
	g_hInstance = hInstance;
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
	InitializeCriticalSection(&g_CriticalDllCache);

	if (!GetCurrentDirectory(_countof(g_WorkingDir), g_WorkingDir)) // Needed for the FileSelectFile() workaround.
		*g_WorkingDir = '\0';
	// Unlike the below, the above must not be Malloc'd because the contents can later change to something
	// as large as MAX_PATH by means of the SetWorkingDir command.
	g_WorkingDirOrig = SimpleHeap::Malloc(g_WorkingDir); // Needed by the Reload command.

	// Set defaults, to be overridden by command line args we receive:
	bool restart_mode = false;

#ifndef AUTOHOTKEYSC
	LPTSTR script_filespec = lpCmdLine ; // Naveen changed from NULL;
#endif

	// The problem of some command line parameters such as /r being "reserved" is a design flaw (one that
	// can't be fixed without breaking existing scripts).  Fortunately, I think it affects only compiled
	// scripts because running a script via AutoHotkey.exe should avoid treating anything after the
	// filename as switches. This flaw probably occurred because when this part of the program was designed,
	// there was no plan to have compiled scripts.
	// 
	// Examine command line args.  Rules:
	// Any special flags (e.g. /force and /restart) must appear prior to the script filespec.
	// The script filespec (if present) must be the first non-backslash arg.
	// All args that appear after the filespec are considered to be parameters for the script
	// and will be added as variables %1% %2% etc.
	// The above rules effectively make it impossible to autostart AutoHotkey.ini with parameters
	// unless the filename is explicitly given (shouldn't be an issue for 99.9% of people).
	TCHAR var_name[32], *param; // Small size since only numbers will be used (e.g. %1%, %2%).
	Var *var;
	bool switch_processing_is_complete = false;
	int script_param_num = 1;

LPTSTR args[4];
//args[0] = __targv[0];   //      name of host program
args[0] = GetCommandLine();
args[1] = nameHinstanceP.argv;  // 1 option such as /Debug  /R /F /STDOUT
args[2] = nameHinstanceP.name;  // name of script to launch
args[3] = nameHinstanceP.args;  // script parameters, all in one string (* char)
int argc = 4;

	for (int i = 1; i < argc; ++i)  //	Naveen changed from:  for (int i = 1; i < __argc; ++i) see above
	{  // Naveen: v6.1 put options in script variables as well
		param = args[i]; // Naveen changed from: __argv[i]; see above
			if (   !(var = g_script.FindOrAddVar(var_name, _stprintf(var_name, _T("%d"), script_param_num)))   )
				return CRITICAL_ERROR;  // Realistically should never happen.
			var->Assign(param);
			++script_param_num;
		
	}   // Naveen: v6.1 only argv needs special processing
	    //              script will do its own parameter parsing

param = nameHinstanceP.argv ; // 
#ifdef CONFIG_DEBUGGER

				g_DebuggerHost = "localhost";
				g_DebuggerPort = "9000";

#endif


#ifndef AUTOHOTKEYSC
	if (script_filespec)// Script filename was explicitly specified, so check if it has the special conversion flag.
	{
		size_t filespec_length = _tcslen(script_filespec);
		if (filespec_length >= CONVERSION_FLAG_LENGTH)
		{
			LPTSTR cp = script_filespec + filespec_length - CONVERSION_FLAG_LENGTH;
			// Now cp points to the first dot in the CONVERSION_FLAG of script_filespec (if it has one).
			if (!_tcsicmp(cp, CONVERSION_FLAG))
				return Line::ConvertEscapeChar(script_filespec);
		}
	}
#endif

	// Like AutoIt2, store the number of script parameters in the script variable %0%, even if it's zero:
	if (   !(var = g_script.FindOrAddVar(_T("0")))   )
		return CRITICAL_ERROR;  // Realistically should never happen.
	var->Assign(script_param_num - 1);

	// N11 
	Var *A_ScriptParams;
	A_ScriptParams = g_script.FindOrAddVar(_T("A_ScriptParams"));	
	A_ScriptParams->Assign(nameHinstanceP.args);

	Var *A_ScriptOptions;
	A_ScriptOptions = g_script.FindOrAddVar(_T("A_ScriptOptions"));	
	A_ScriptOptions->Assign(nameHinstanceP.argv);

	global_init(*g);  // Set defaults prior to the below, since below might override them for AutoIt2 scripts.

// Set up the basics of the script:
#ifdef AUTOHOTKEYSC
	if (g_script.Init(*g, "", restart_mode) != OK) 
		return CRITICAL_ERROR;
#else //HotKeyIt: Go different init routine when file does not exist and assume this is a script to load
	if (!nameHinstanceP.istext)
	{
		if (g_script.Init(*g, script_filespec, restart_mode,0) != OK)  // Set up the basics of the script, using the above.
			return CRITICAL_ERROR;
	}
	else
	{
		if (g_script.Init(*g,_T(""),restart_mode,hInstance) != OK)  // Set up the basics of the script.
			return CRITICAL_ERROR;
	}
#endif // AUTOHOTKEYSC

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
#ifndef USRDLL	
	if (g_AllowOnlyOneInstance == ALLOW_MULTI_INSTANCE && IS_PERSISTENT)
		g_AllowOnlyOneInstance = SINGLE_INSTANCE_PROMPT;
#else
#ifndef MINIDLL
	if (g_AllowOnlyOneInstance == ALLOW_MULTI_INSTANCE)
		g_AllowOnlyOneInstance = SINGLE_INSTANCE_PROMPT;
#endif
#endif

#ifndef MINIDLL
/*
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
				if (MsgBox(_T("An older instance of this script is already running.  Replace it with this")
					_T(" instance?\nNote: To avoid this message, see #SingleInstance in the help file.")
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
		terminateDll();
		//PostMessage(w_existing, WM_CLOSE, 0, 0);

		// Wait for it to close before we continue, so that it will deinstall any
		// hooks and unregister any hotkeys it has:
		int interval_count;
		for (interval_count = 0; ; ++interval_count)
		{
			Sleep(10);  // No need to use MsgSleep() in this case.
			if (!IsWindow(w_existing))
				break;  // done waiting.
			if (interval_count == 100)
			{
				// This can happen if the previous instance has an OnExit subroutine that takes a long
				// time to finish, or if it's waiting for a network drive to timeout or some other
				// operation in which it's thread is occupied.
				if (MsgBox(_T("Could not close the previous instance of this script.  Keep waiting?"), 4) == IDNO)
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
*/
#endif

	// Create all our windows and the tray icon.  This is done after all other chances
	// to return early due to an error have passed, above.
	if (g_script.CreateWindows() != OK)
		return CRITICAL_ERROR;

	// At this point, it is nearly certain that the script will be executed.

	// v1.0.48.04: Turn off buffering on stdout so that "FileAppend, Text, *" will write text immediately
	// rather than lazily. This helps debugging, IPC, and other uses, probably with relatively little
	// impact on performance given the OS's built-in caching.  I looked at the source code for setvbuf()
	// and it seems like it should execute very quickly.  Code size seems to be about 75 bytes.
	setvbuf(stdout, NULL, _IONBF, 0); // Must be done PRIOR to writing anything to stdout.

#ifndef MINIDLL
	if (g_MaxHistoryKeys && (g_KeyHistory = (KeyHistoryItem *)malloc(g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
		ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
	//else leave it NULL as it was initialized in globaldata.
#endif
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
			GetProcAddress(GetModuleHandle(_T("comctl32")), "InitCommonControlsEx"); // LoadLibrary shouldn't be necessary because comctl32 in linked by compiler.
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

	// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
	// top part (the auto-execute part) of the script so that they will be in effect even if the
	// top part is something that's very involved and requires user interaction:
#ifndef MINIDLL
	Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
	//Hotkey::InstallKeybdHook();
	//Hotkey::InstallMouseHook();
	//if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
	//	AddRemoveHooks(3);
#endif
	g_script.mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
	Sleep(20);
	//free(nameHinstanceP.name);

	Var *clipboard_var = g_script.FindOrAddVar(_T("Clipboard")); // Add it if it doesn't exist, in case the script accesses "Clipboard" via a dynamic variable.
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
	LPTSTR fileName = a.name;
	OldWinMain(hInstance, 0, fileName, 0);	
	_endthreadex( (DWORD)EARLY_RETURN );  
    return 0;
}


EXPORT int ahkTerminate(bool kill)
{
	int lpExitCode = 0;
	GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
	if (!kill)
	for (int i = 0; g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259) && i < 10; i++)
	{
		PostMessage(g_hWnd, AHK_EXIT_BY_SINGLEINSTANCE, EARLY_EXIT, 0);
		Sleep(50);
		GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
	}
	if (lpExitCode != 0 && lpExitCode != 259)
		return 0;
	Line::sSourceFileCount = 0;
	global_clear_state(*g);
	g_script.Destroy();
	TerminateThread(hThread, (DWORD)EARLY_EXIT);
	CloseHandle(hThread);
	g_script.mIsReadyToExecute = false;
	return 0;
}

void WaitIsReadyToExecute()
{
	 int lpExitCode = 0;
	 while (!g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259))
	 {
		 Sleep(10);
		 GetExitCodeThread(hThread,(LPDWORD)&lpExitCode);
	 }
}


unsigned runThread()
{
	if (hThread)
 		ahkTerminate(0);
	hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, 0 );
	//hThread = (HANDLE)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&runScript,&nameHinstanceP,0,(LPDWORD)&threadID);
	//hThread = AfxBeginThread(&runScript,&nameHinstanceP,THREAD_PRIORITY_NORMAL,0,0,NULL);
	WaitIsReadyToExecute();
	return (unsigned int)hThread;
}

EXPORT unsigned int ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR args)
{
	nameHinstanceP.name = fileName ;
	nameHinstanceP.argv = argv ;
	nameHinstanceP.args = args ;
	nameHinstanceP.istext = 0;
	return runThread();
}

// HotKeyIt ahktextdll
EXPORT unsigned int ahktextdll(LPTSTR fileName, LPTSTR argv, LPTSTR args)
{
	nameHinstanceP.name = fileName ;
	nameHinstanceP.argv = argv ;
	nameHinstanceP.args = args ;
	nameHinstanceP.istext = 1;
	return runThread();
}


void reloadDll()
{
	unsigned threadID;
	Line::sSourceFileCount = 0;
	g_script.Destroy();
	hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, &threadID );
	_endthreadex( (DWORD)EARLY_RETURN ); 
}

ResultType terminateDll()
{
	Line::sSourceFileCount = 0;
	g_script.Destroy();
	_endthreadex( (DWORD)EARLY_EXIT );
	return EARLY_EXIT;
}


EXPORT int ahkReload()
{
	ahkTerminate(0);
	hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, 0 );
	return 0;
}

EXPORT BOOL ahkReady() // HotKeyIt check if dll is ready to execute
{
	return g_script.mIsReadyToExecute;
}
#endif




