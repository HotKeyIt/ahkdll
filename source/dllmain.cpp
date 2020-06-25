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
#ifdef _USRDLL
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "window.h" // For MsgBox() & SetForegroundLockTimeout()
#include "TextIO.h"

#include "windows.h"  // N11
#include "exports.h"  // N11
#include <process.h>  // N11


#include <objbase.h> // COM
#include "ComServer_i.h"
#include "ComServer_i.c"
#include <atlbase.h> // CComBSTR
#include "Registry.h"
#include "ComServerImpl.h"
#include "MemoryModule.h"
//#include <string>

// General note:
// The use of Sleep() should be avoided *anywhere* in the code.  Instead, call MsgSleep().
// The reason for this is that if the keyboard or mouse hook is installed, a straight call
// to Sleep() will cause user keystrokes & mouse events to lag because the message pump
// (GetMessage() or PeekMessage()) is the only means by which events are ever sent to the
// hook functions.


static LPTSTR aDefaultDllScript = _T("#Persistent\n#NoTrayIcon");
static LPTSTR scriptstring = NULL;
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
unsigned __stdcall runScript( void* pArguments );

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
		g_hInstance = (HINSTANCE)hInstance;
		g_hMemoryModule = (HMODULE)lpvReserved;

		/*
		if (!IsBadReadPtr(g_hMemoryModule,1) && (((PMEMORYMODULE)lpvReserved)->modules != NULL))
			g_hMSVCR = ((PMEMORYMODULE)lpvReserved)->modules[0];
		*/
		InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
		InitializeCriticalSection(&g_CriticalHeapBlocks); // used to block memory freeing in case of timeout in ahkTerminate so no corruption happens when both threads try to free Heap.
		InitializeCriticalSection(&g_CriticalAhkFunction); // used to call a function in multithreading environment.
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
		 if (g_hThread)
		 {
			 int lpExitCode = 0;
			 GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
			 if ( lpExitCode == 259 )
				CloseHandle(g_hThread);
		 }
		 g_script.~Script();
		 if (scriptstring)
			free(scriptstring);
		 if (Line::sMaxSourceFiles)
			free(Line::sSourceFile);
#ifdef _DEBUG
		 free(g_Debugger.mStack.mBottom);
#endif
#ifndef MINIDLL
		 if (g_script.mTrayMenu)
			 g_script.ScriptDeleteMenu(g_script.mTrayMenu);
		 free(g_KeyHistory);
#endif
		 if (g_hWinAPI)
		 {
			 free(g_hWinAPI);
			 free(g_hWinAPIlowercase);
		 }
		 for(int i = 0;i < FUNC_LIB_COUNT;i++)
		 {
			 if (sLib[i].path && *sLib[i].path)
				free(sLib[i].path);
		 }
		 DeleteCriticalSection(&g_CriticalHeapBlocks); // g_CriticalHeapBlocks is used in simpleheap for thread-safety.
		 DeleteCriticalSection(&g_CriticalRegExCache); // g_CriticalRegExCache is used elsewhere for thread-safety.
		 DeleteCriticalSection(&g_CriticalAhkFunction); // used to call a function in multithreading environment.
		 break;
	 }
 case DLL_THREAD_DETACH:
 break;
 }

 return(TRUE); // a FALSE will abort the DLL attach
 }
 
int WINAPI OldWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifndef MINIDLL
	// Lastly (after the above have been initialized), anything that can fail:
	if ( !g_script.mTrayMenu && !(g_script.mTrayMenu = g_script.AddMenu(_T("Tray")))   ) // realistically never happens
	{
		g_script.ScriptError(_T("No tray mem"));
		g_script.ExitApp(EXIT_DESTROY);
	}
	else
		g_script.mTrayMenu->mIncludeStandardItems = true;
#endif
	// Init any globals not in "struct g" that need it:
	g_MainThreadID = GetCurrentThreadId();

	HMODULE advapi32 = LoadLibrary(_T("advapi32.dll"));
	g_CryptEncrypt = (MyCryptEncrypt)GetProcAddress(advapi32, "CryptEncrypt");
	g_CryptDecrypt = (MyCryptDecrypt)GetProcAddress(advapi32, "CryptDecrypt");
#ifdef _DEBUG
	g_hResource = FindResource(g_hInstance, _T("AHK"), RT_RCDATA);
#else
	if (g_hMemoryModule)
		g_hResource = (HRSRC)MemoryFindResource(g_hMemoryModule, _T("E4847ED08866458F8DD35F94B37001C0"), RT_RCDATA);
	else
		g_hResource = FindResource(g_hInstance, _T("E4847ED08866458F8DD35F94B37001C0"), RT_RCDATA);
#endif
	
	UpdateWorkingDir(); // Needed for the FileSelectFile() workaround.
	g_WorkingDirOrig = SimpleHeap::Malloc(const_cast<LPTSTR>(g_WorkingDir.GetString())); // Needed by the Reload command.

	// Set defaults, to be overridden by command line args we receive:
	bool restart_mode = false;

	LPTSTR script_filespec = lpCmdLine ; // Naveen changed from NULL;

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

	int dllargc = 0;
#ifndef _UNICODE
	LPWSTR wargv = (LPWSTR) _malloca((_tcslen(nameHinstanceP.args)+1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8,0,nameHinstanceP.args,-1,wargv,(_tcslen(nameHinstanceP.args)+1)*sizeof(WCHAR));
	LPWSTR *dllargv = CommandLineToArgvW(wargv,&dllargc);
	_freea(wargv);
#else
	LPWSTR *dllargv = CommandLineToArgvW(nameHinstanceP.args,&dllargc);
#endif
	int first_script_param = dllargc;
	int i;
	if (*nameHinstanceP.args) // Only process if parameters were given
	for (i = 0; i < dllargc; ++i) // Start at 1 because 0 contains the program name.
	{
#ifndef _UNICODE
		param = (TCHAR *) _alloca(wcslen(dllargv[i])+1);
		WideCharToMultiByte(CP_ACP,0,dllargv[i],-1,param,(wcslen(dllargv[i])+1),0,0);
#else
		param = dllargv[i]; // For performance and convenience.
#endif
		if (switch_processing_is_complete) // All args are now considered to be input parameters for the script.
		{
			if (   !(var = g_script.FindOrAddVar(var_name, _stprintf(var_name, _T("%d"), script_param_num)))   )
			{
				g_Reloading = false;
				return CRITICAL_ERROR;  // Realistically should never happen.
			}
			var->Assign(param);
			++script_param_num;
		}
		// Insist that switches be an exact match for the allowed values to cut down on ambiguity.
		// For example, if the user runs "CompiledScript.exe /find", we want /find to be considered
		// an input parameter for the script rather than a switch:
		else if (!_tcsicmp(param, _T("/R")) || !_tcsicmp(param, _T("/restart")))
			restart_mode = true;
		else if (!_tcsicmp(param, _T("/F")) || !_tcsicmp(param, _T("/force")))
			g_ForceLaunch = true;
		else if (!_tcsicmp(param, _T("/ErrorStdOut")))
			g_script.mErrorStdOut = true;
		else if (!_tcsicmp(param, _T("/iLib"))) // v1.0.47: Build an include-file so that ahk2exe can include library functions called by the script.
		{
			++i; // Consume the next parameter too, because it's associated with this one.
			if (i >= dllargc) // Missing the expected filename parameter.
			{
				g_Reloading = false;
				return CRITICAL_ERROR;
			}
			// For performance and simplicity, open/create the file unconditionally and keep it open until exit.
			g_script.mIncludeLibraryFunctionsThenExit = new TextFile;
#ifndef _UNICODE
			param = (TCHAR *) _alloca(wcslen(dllargv[i])+1);
			WideCharToMultiByte(CP_ACP,0,dllargv[i],-1,param,(wcslen(dllargv[i])+1),0,0);
			if (!g_script.mIncludeLibraryFunctionsThenExit->Open(param, TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
#else
			if (!g_script.mIncludeLibraryFunctionsThenExit->Open(dllargv[i], TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
#endif			
			{
				g_Reloading = false;
				return CRITICAL_ERROR;
			}
		}
		else if (!_tcsicmp(param, _T("/E")) || !_tcsicmp(param, _T("/Execute")))
		{
			g_hResource = NULL; // Execute script from File. Override compiled, A_IsCompiled will also report 0
		}
		else if (!_tcsnicmp(param, _T("/CP"), 3)) // /CPnnn
		{
			// Default codepage for the script file, NOT the default for commands used by it.
			g_DefaultScriptCodepage = ATOU(param + 3);
		}
#ifdef CONFIG_DEBUGGER
		// Allow a debug session to be initiated by command-line.
		else if (!g_Debugger.IsConnected() && !_tcsnicmp(param, _T("/Debug"), 6) && (param[6] == '\0' || param[6] == '='))
		{
			if (param[6] == '=')
			{
				param += 7;

				LPTSTR c = _tcsrchr(param, ':');

				if (c)
				{
					StringTCharToChar(param, g_DebuggerHost, (int)(c-param));
					StringTCharToChar(c + 1, g_DebuggerPort);
				}
				else
				{
					StringTCharToChar(param, g_DebuggerHost);
					g_DebuggerPort = "9000";
				}
			}
			else
			{
				g_DebuggerHost = "127.0.0.1";
				g_DebuggerPort = "9000";
			}
			// The actual debug session is initiated after the script is successfully parsed.
		}
#endif
		else // since this is not a recognized switch, the end of the [Switches] section has been reached (by design).
		{
			switch_processing_is_complete = true;  // No more switches allowed after this point.
			--i; // Make the loop process this item again so that it will be treated as a script param.
			first_script_param = i + 1;
		}
	}
	
	
	// Like AutoIt2, store the number of script parameters in the script variable %0%, even if it's zero:
	if (   !(var = g_script.FindOrAddVar(_T("0")))   )
	{
		g_Reloading = false;
		return CRITICAL_ERROR;  // Realistically should never happen.
	}
	var->Assign(script_param_num - 1);

	if (Var *var = g_script.FindOrAddVar(_T("A_Args"), 6, VAR_DECLARE_SUPER_GLOBAL))
	{
		// Store the remaining args in an array and assign it to "Args".
		// If there are no args, assign an empty array.
#ifndef _UNICODE
		Object *args = Object::CreateFromArgVW(dllargv + first_script_param, dllargc - first_script_param);
#else
		Object *args = Object::CreateFromArgV(dllargv + first_script_param, dllargc - first_script_param);
#endif
		if (!args)
			return CRITICAL_ERROR;  // Realistically should never happen.
		var->AssignSkipAddRef(args);
	}
	else
		return CRITICAL_ERROR;

	LocalFree(dllargv); // free memory allocated by CommandLineToArgvW

	// N11 

	Var *A_ScriptOptions;
	A_ScriptOptions = g_script.FindOrAddVar(_T("A_ScriptOptions"),0,VAR_GLOBAL|VAR_SUPER_GLOBAL);	
	A_ScriptOptions->Assign(nameHinstanceP.argv);

	global_init(*g);  // Set defaults prior to the below, since below might override them for AutoIt2 scripts.

// Set up the basics of the script:
	if (g_script.Init(*g, script_filespec, restart_mode,hInstance,g_hResource ? 0 : (bool)nameHinstanceP.istext) != OK)  // Set up the basics of the script, using the above.
	{
		g_Reloading = false;
		return CRITICAL_ERROR;
	}
	// Set g_default now, reflecting any changes made to "g" above, in case AutoExecSection(), below,
	// never returns, perhaps because it contains an infinite loop (intentional or not):
	CopyMemory(&g_default, g, sizeof(global_struct));

	//if (nameHinstanceP.istext)
	//	GetCurrentDirectory(MAX_PATH, g_script.mFileDir);
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
	LineNumberType load_result = (g_hResource || !nameHinstanceP.istext) ? g_script.LoadFromFile() : g_script.LoadFromText(script_filespec);
#endif
	
	if (load_result == LOADING_FAILED) // Error during load (was already displayed by the function call).
	{
		g_Reloading = false;
		if (g_script.mIncludeLibraryFunctionsThenExit)
			g_script.mIncludeLibraryFunctionsThenExit->Close(); // Flush its buffer to disk.
		return CRITICAL_ERROR;  // Should return this value because PostQuitMessage() also uses it.
	}
	if (!load_result) // LoadFromFile() relies upon us to do this check.  No lines were loaded, so we're done.
	{
		g_Reloading = false;
		return 0;
	}

	// Unless explicitly set to be non-SingleInstance via SINGLE_INSTANCE_OFF or a special kind of
	// SingleInstance such as SINGLE_INSTANCE_REPLACE and SINGLE_INSTANCE_IGNORE, persistent scripts
	// and those that contain hotkeys/hotstrings are automatically SINGLE_INSTANCE_PROMPT as of v1.0.16:
#ifndef MINIDLL
/*
	if (g_AllowOnlyOneInstance == ALLOW_MULTI_INSTANCE)
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
	{
		g_Reloading = false;
		return CRITICAL_ERROR;
	}
	// Set AutoHotkey.dll its main window (ahk_class AutoHotkey) bottom so it does not receive Reload or ExitApp Message send e.g. when Reload message is sent.
	SetWindowPos(g_hWnd,HWND_BOTTOM,0,0,0,0,SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSENDCHANGING|SWP_NOSIZE);
	// At this point, it is nearly certain that the script will be executed.

	// v1.0.48.04: Turn off buffering on stdout so that "FileAppend, Text, *" will write text immediately
	// rather than lazily. This helps debugging, IPC, and other uses, probably with relatively little
	// impact on performance given the OS's built-in caching.  I looked at the source code for setvbuf()
	// and it seems like it should execute very quickly.  Code size seems to be about 75 bytes.
	setvbuf(stdout, NULL, _IONBF, 0); // Must be done PRIOR to writing anything to stdout.

#ifndef MINIDLL
	if (g_MaxHistoryKeys && (g_KeyHistory = (KeyHistoryItem *)realloc(g_KeyHistory,g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
		ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
	//else leave it NULL as it was initialized in globaldata.
#endif
	// MSDN: "Windows XP: If a manifest is used, InitCommonControlsEx is not required."
	// Therefore, in case it's a high overhead call, it's not done on XP or later:
	if (!g_os.IsWinXPorLater())
	{
		// Since InitCommonControls() is apparently incapable of initializing DateTime and MonthCal
		// controls, InitCommonControlsEx() must be called.  But since Ex() requires comctl32.dll
		INITCOMMONCONTROLSEX icce;
		icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icce.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES; // ICC_WIN95_CLASSES is equivalent to calling InitCommonControls().
		InitCommonControlsEx(&icce);
	}

#ifdef CONFIG_DEBUGGER
	// Initiate debug session now if applicable.
	if (!g_DebuggerHost.IsEmpty() && g_Debugger.Connect(g_DebuggerHost, g_DebuggerPort) == DEBUGGER_E_OK)
	{
		g_Debugger.ProcessCommands();
	}
#endif
	/* // Use ExceptionHandler only in main process
	// set exception filter to disable hook before exception occures to avoid system/mouse freeze
	// also when dll will crash, it will only exit dll thread and try to destroy it, leaving the exe process running
	g_ExceptionHandler = AddVectoredExceptionHandler(NULL,DisableHooksOnException);
	*/


	// set exception filter to disable hook before exception occures to avoid system/mouse freeze
	// specify 1 so dll handler runs before exe handler
	g_ExceptionHandler = AddVectoredExceptionHandler(1,DisableHooksOnException);
#ifndef MINIDLL

	// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
	// top part (the auto-execute part) of the script so that they will be in effect even if the
	// top part is something that's very involved and requires user interaction:
	Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
	//Hotkey::InstallKeybdHook();
	//Hotkey::InstallMouseHook();
	//if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
	//	AddRemoveHooks(3);
	g_HSSameLineAction = false; // `#Hotstring E` should not affect Hotstring().
#endif
	
	g_script.mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
	//Sleep(20);
	g_Reloading = false;
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
	// 2) The AutoExec function uses the Exit or ExitApp command to terminate the script.
	// 3) The script isn't persistent and its last line is reached (in which case an ExitApp is implicit).

	// Call it in this special mode to kick off the main event loop.
	// Be sure to pass something >0 for the first param or it will
	// return (and we never want this to return):
	MsgSleep(SLEEP_INTERVAL, WAIT_FOR_MESSAGES);
	return 0; // Never executed; avoids compiler warning.
}


EXPORT BOOL ahkTerminate(int timeout = 0)
{
	DWORD lpExitCode = 0;
	if (g_hThread == 0)
		return 0;
	g_AllowInterruption = FALSE;
	GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
	DWORD tickstart = GetTickCount();
	DWORD timetowait = timeout < 0 ? timeout * -1 : timeout;
	for (; g_hThread && g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259) && (timeout == 0 || timetowait > (GetTickCount()-tickstart));)
	{
		SendMessageTimeout(g_hWnd, AHK_EXIT_BY_SINGLEINSTANCE, OK, 0,timeout < 0 ? SMTO_NORMAL : SMTO_NOTIMEOUTIFNOTHUNG,SLEEP_INTERVAL * 3,0);
		Sleep(100); // give it a bit time to exit thread
	}
	if (g_script.mIsReadyToExecute || g_hThread)
	{
		g_script.Destroy();
		TerminateThread(g_hThread, (DWORD)EARLY_EXIT);
		CloseHandle(g_hThread);
		g_hThread = NULL;
	}
	g_AllowInterruption = TRUE;
	return 0;
}

// Naveen: v1. runscript() - runs the script in a separate thread compared to host application.
unsigned __stdcall runScript( void* pArguments )
{
	OleInitialize(NULL);
	int result = OldWinMain(nameHinstanceP.hInstanceP, 0, nameHinstanceP.name, 0);
	g_script.Destroy();
	g_hThread = NULL;
	_endthreadex( result);  
    return 0;
}


void WaitIsReadyToExecute()
{
	 int lpExitCode = 0;
	 while (g_hThread && !g_script.mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259))
	 {
		Sleep(10);
		GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
	 }
	 if (g_hThread && !g_script.mIsReadyToExecute)
	 {
		CloseHandle(g_hThread);
		g_hThread = NULL;
		SetLastError(lpExitCode);
	 }
}


unsigned runThread()
{
	if (g_hThread && g_script.mIsReadyToExecute)
	{	// Small check to be done to make sure we do not start a new thread before the old is closed
		Sleep(50);
		int lpExitCode = 0;
		GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
		if ((lpExitCode == 0 || lpExitCode == 259) && g_script.mIsReadyToExecute)
			Sleep(50); // make sure the script is not about to be terminated, because this might lead to problems
		if (g_hThread && g_script.mIsReadyToExecute)
 			ahkTerminate(0);
	}
	g_hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, NULL, 0, 0 );
	WaitIsReadyToExecute();
	return (unsigned int)g_hThread;
}

int setscriptstrings(LPTSTR fileName, LPTSTR argv, LPTSTR args)
{
	LPTSTR newstring = (LPTSTR)realloc(scriptstring,(_tcslen(fileName)+_tcslen(argv)+_tcslen(args)+3)*sizeof(TCHAR));
	if (!newstring)
		return 1;
	scriptstring = newstring;
	_tcscpy(scriptstring,fileName);
	_tcscpy(scriptstring + _tcslen(fileName) + 1,argv);
	_tcscpy(scriptstring + _tcslen(fileName) + _tcslen(argv) + 2,args);
	nameHinstanceP.name = scriptstring;
	nameHinstanceP.argv = scriptstring + _tcslen(fileName) + 1 ;
	nameHinstanceP.args = scriptstring + _tcslen(fileName) + _tcslen(argv) + 2 ;
	return 0;
}

EXPORT UINT_PTR ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR args)
{
	if (setscriptstrings(fileName && !IsBadReadPtr(fileName,1) && *fileName ? fileName : aDefaultDllScript, argv && !IsBadReadPtr(argv,1) && *argv ? argv : _T(""), args && !IsBadReadPtr(args,1) && *args ? args : _T("")))
		return 0;
	nameHinstanceP.istext = *fileName ? 0 : 1;
	return runThread();
}

// HotKeyIt ahktextdll
EXPORT UINT_PTR ahktextdll(LPTSTR fileName, LPTSTR argv, LPTSTR args)
{
	if (setscriptstrings(fileName && !IsBadReadPtr(fileName,1) && *fileName ? fileName : aDefaultDllScript, argv && !IsBadReadPtr(argv,1) && *argv ? argv : _T(""), args && !IsBadReadPtr(args,1) && *args ? args : _T("")))
		return 0;
	nameHinstanceP.istext = 1;
	return runThread();
}

void reloadDll()
{
	g_script.Destroy();
	HANDLE oldhThread = g_hThread;
	g_hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, 0 );
	g_AllowInterruption = TRUE;
	CloseHandle(oldhThread);
	_endthreadex( (DWORD)EARLY_EXIT );
}

ResultType terminateDll(int aExitCode)
{
	g_script.Destroy();
	g_AllowInterruption = TRUE;
	CloseHandle(g_hThread);
	g_hThread = NULL;
	_endthreadex( (DWORD)aExitCode );
	return (ResultType)aExitCode;
}

EXPORT int ahkReload(int timeout = 0)
{
	ahkTerminate(timeout);
	g_hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, 0 );
	return 0;
}

EXPORT int ahkReady() // HotKeyIt check if dll is ready to execute
{
	return g_script.mIsReadyToExecute || g_Reloading || g_Loading;
}

#ifndef MINIDLL

// COM Implementation //
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

// Friendly name of component
const char g_szFriendlyName[] = "AutoHotkey Script" ;

// Version-independent ProgID
const char g_szVerIndProgID[] = "AutoHotkey.Script" ;

// ProgID
const char g_szProgID[] = "AutoHotkey.Script.1" ;

#ifdef _WIN64
const char g_szFriendlyNameOptional[] = "AutoHotkey Script X64" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey.Script.X64" ;
const char g_szProgIDOptional[] = "AutoHotkey.Script.X64.1" ;
#else
#ifdef _UNICODE
const char g_szFriendlyNameOptional[] = "AutoHotkey Script UNICODE" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey.Script.UNICODE" ;
const char g_szProgIDOptional[] = "AutoHotkey.Script.UNICODE.1" ;
#else
const char g_szFriendlyNameOptional[] = "AutoHotkey Script ANSI" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey.Script.ANSI" ;
const char g_szProgIDOptional[] = "AutoHotkey.Script.ANSI.1" ;
#endif // UNICODE
#endif // WIN64

//
// Constructor
//
CoCOMServer::CoCOMServer() : m_cRef(1)

{ 
	InterlockedIncrement(&g_cComponents) ; 

	m_ptinfo = NULL;
	LoadTypeInfo(&m_ptinfo, LIBID_AutoHotkey, IID_ICOMServer, 0);
}

//
// Destructor
//
CoCOMServer::~CoCOMServer() 
{ 
	InterlockedDecrement(&g_cComponents) ; 
}

//
// IUnknown implementation
//
HRESULT __stdcall CoCOMServer::QueryInterface(const IID& iid, void** ppv)
{    
	if (iid == IID_IUnknown || iid == IID_ICOMServer || iid == IID_IDispatch)
	{
		*ppv = static_cast<ICOMServer*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

ULONG __stdcall CoCOMServer::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall CoCOMServer::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}


//
// ICOMServer implementation
//

LPTSTR Variant2T(VARIANT var,LPTSTR buf)
{
	USES_CONVERSION;
	if (var.vt == VT_BYREF+VT_VARIANT)
		var = *var.pvarVal;
	if (var.vt == VT_ERROR)
		return _T("");
	else if (var.vt==VT_BSTR)
		return OLE2T(var.bstrVal);
	else if (var.vt==VT_I2 || var.vt==VT_I4 || var.vt==VT_I8)
#ifdef _WIN64
		return _ui64tot(var.uintVal,buf,10);
#else
		return _ultot(var.uintVal,buf,10);
#endif
	return _T("");
}

unsigned int Variant2I(VARIANT var)
{
	USES_CONVERSION;
	if (var.vt == VT_BYREF+VT_VARIANT)
		var = *var.pvarVal;
	if (var.vt == VT_ERROR)
		return 0;
	else if (var.vt == VT_BSTR)
		return ATOI(OLE2T(var.bstrVal));
	else //if (var.vt==VT_I2 || var.vt==VT_I4 || var.vt==VT_I8)
		return var.uintVal;
}

HRESULT __stdcall CoCOMServer::ahktextdll(/*in,optional*/VARIANT script,/*in,optional*/VARIANT options,/*in,optional*/VARIANT params,/*out*/UINT_PTR* hThread)
{
	USES_CONVERSION;
	TCHAR buf1[MAX_INTEGER_SIZE],buf2[MAX_INTEGER_SIZE],buf3[MAX_INTEGER_SIZE];
	if (hThread==NULL)
		return ERROR_INVALID_PARAMETER;
	*hThread = com_ahktextdll(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script,buf1)
							,options.vt == VT_BSTR ? OLE2T(options.bstrVal) : Variant2T(options,buf2)
							,params.vt == VT_BSTR ? OLE2T(params.bstrVal) : Variant2T(params,buf3));
	return S_OK;
}

HRESULT __stdcall CoCOMServer::ahkdll(/*in,optional*/VARIANT filepath,/*in,optional*/VARIANT options,/*in,optional*/VARIANT params,/*out*/UINT_PTR* hThread)
{
	USES_CONVERSION;
	TCHAR buf1[MAX_INTEGER_SIZE],buf2[MAX_INTEGER_SIZE],buf3[MAX_INTEGER_SIZE];
	if (hThread==NULL)
		return ERROR_INVALID_PARAMETER;
	*hThread = com_ahkdll(filepath.vt == VT_BSTR ? OLE2T(filepath.bstrVal) : Variant2T(filepath,buf1)
							,options.vt == VT_BSTR ? OLE2T(options.bstrVal) : Variant2T(options,buf2)
							,params.vt == VT_BSTR ? OLE2T(params.bstrVal) : Variant2T(params,buf3));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkPause(/*in,optional*/VARIANT aChangeTo,/*out*/BOOL* paused)
{
	USES_CONVERSION;
	if (paused==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*paused = com_ahkPause(aChangeTo.vt == VT_BSTR ? OLE2T(aChangeTo.bstrVal) : Variant2T(aChangeTo,buf));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkReady(/*out*/BOOL* ready)
{
	if (ready==NULL)
		return ERROR_INVALID_PARAMETER;
	*ready = com_ahkReady();
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkIsUnicode(/*out*/BOOL* IsUnicode)
{
	if (IsUnicode==NULL)
		return ERROR_INVALID_PARAMETER;
	*IsUnicode = com_ahkIsUnicode();
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkFindLabel(/*in*/VARIANT aLabelName,/*out*/UINT_PTR* aLabelPointer)
{
	USES_CONVERSION;
	if (aLabelPointer==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*aLabelPointer = com_ahkFindLabel(aLabelName.vt == VT_BSTR ? OLE2T(aLabelName.bstrVal) : Variant2T(aLabelName,buf));
	return S_OK;
}

void TokenToVariant(ExprTokenType &aToken, VARIANT &aVar, BOOL aVarIsArg);

HRESULT __stdcall CoCOMServer::ahkgetvar(/*in*/VARIANT name,/*[in,optional]*/ VARIANT getVar,/*out*/VARIANT *result)
{
	USES_CONVERSION;
	if (result==NULL)
		return ERROR_INVALID_PARAMETER;
	//USES_CONVERSION;
	TCHAR buf[MAX_INTEGER_SIZE];
	Var *var;
	ExprTokenType aToken ;
	
	var = g_script.FindVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name,buf)) ;
	var->ToTokenSkipAddRef(aToken);
    VariantInit(result);
   // CComVariant b ;
	VARIANT b ; 
	TokenToVariant(aToken, b, FALSE);
	return VariantCopy(result, &b) ;
	// return S_OK ;
	// return b.Detach(result);
}

void AssignVariant(Var &aArg, VARIANT &aVar, bool aRetainVar = true);

HRESULT __stdcall CoCOMServer::ahkassign(/*in*/VARIANT name, /*in*/VARIANT value,/*out*/int* success)
{
   USES_CONVERSION;
	 if (success==NULL)
      return ERROR_INVALID_PARAMETER;
   TCHAR namebuf[MAX_INTEGER_SIZE];
   Var *var;
   if (   !(var = g_script.FindOrAddVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name,namebuf)))   )
      return ERROR_INVALID_PARAMETER;  // Realistically should never happen.
   AssignVariant(*var, value, false);
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkExecuteLine(/*[in,optional]*/ VARIANT line,/*[in,optional]*/ VARIANT aMode,/*[in,optional]*/ VARIANT wait,/*[out, retval]*/ UINT_PTR* pLine)
{
	if (pLine==NULL)
		return ERROR_INVALID_PARAMETER;
	*pLine = com_ahkExecuteLine(Variant2I(line),Variant2I(aMode),Variant2I(wait));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkLabel(/*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ BOOL* success)
{
	USES_CONVERSION;
	if (success==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_ahkLabel(aLabelName.vt == VT_BSTR ? OLE2T(aLabelName.bstrVal) : Variant2T(aLabelName,buf)
							,Variant2I(nowait));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkFindFunc(/*[in]*/ VARIANT FuncName,/*[out, retval]*/ UINT_PTR* pFunc)
{
	USES_CONVERSION;
	if (pFunc==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*pFunc = com_ahkFindFunc(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName,buf));
	return S_OK;
}

VARIANT ahkFunctionVariant(LPTSTR func, VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10, int sendOrPost);
HRESULT __stdcall CoCOMServer::ahkFunction(/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal)
{
	USES_CONVERSION;
	if (returnVal==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE] ;
	// CComVariant b ;
	VARIANT b ;
	b = ahkFunctionVariant(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName,buf)
							, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, 1);
	 VariantInit(returnVal);
	return VariantCopy(returnVal, &b) ;
	 // return b.Detach(returnVal);			
}
HRESULT __stdcall CoCOMServer::ahkPostFunction(/*[in]*/ VARIANT FuncName,VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ int* returnVal)
{
	USES_CONVERSION;
  	if (returnVal==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE] ;
	// CComVariant b ;
	VARIANT b ;
	b = ahkFunctionVariant(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName,buf)
							, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, 0);
	return 0;		
}

HRESULT __stdcall CoCOMServer::addScript(/*[in]*/ VARIANT script,/*[in,optional]*/ VARIANT waitexecute,/*[out, retval]*/ UINT_PTR* success)
{
	USES_CONVERSION;
	if (success==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_addScript(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script,buf),Variant2I(waitexecute));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::addFile(/*[in]*/ VARIANT filepath,/*[in,optional]*/ VARIANT waitexecute,/*[out, retval]*/ UINT_PTR* success)
{
	USES_CONVERSION;
	if (success==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_addFile(filepath.vt == VT_BSTR ? OLE2T(filepath.bstrVal) : Variant2T(filepath,buf)
							,Variant2I(waitexecute));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkExec(/*[in]*/ VARIANT script,/*[out, retval]*/ int* success)
{
	USES_CONVERSION;
	if (success==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_ahkExec(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script,buf));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkTerminate(/*[in,optional]*/ VARIANT kill,/*[out, retval]*/ int* success)
{
	if (success==NULL)
		return ERROR_INVALID_PARAMETER;
	*success = com_ahkTerminate(Variant2I(kill));
	return S_OK;
}

HRESULT __stdcall CoCOMServer::ahkReload(/*[in,optional]*/ VARIANT timeout)
{
	com_ahkReload(Variant2I(timeout));
	return S_OK;
}
HRESULT CoCOMServer::LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID &libid, const CLSID &iid, LCID lcid)
{
   HRESULT hr;
   LPTYPELIB ptlib = NULL;
   LPTYPEINFO ptinfo = NULL;

   *pptinfo = NULL;
   
   // Load type library.
   hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptlib);
   if (FAILED(hr))
   { // search for TypeLib in current dll
	  WCHAR buf[MAX_PATH * sizeof(WCHAR)]; // LoadTypeLibEx needs Unicode string
	  if (GetModuleFileNameW(g_hInstance, buf, _countof(buf)))
		  hr = LoadTypeLibEx(buf,REGKIND_NONE,&ptlib);
	  else // MemoryModule, search troug g_ListOfMemoryModules and use temp file to extract and load TypeLib file
	  {
		  HMEMORYMODULE hmodule = (HMEMORYMODULE)(g_hMemoryModule);
		  HMEMORYRSRC res = MemoryFindResource(hmodule, MAKEINTRESOURCE(1), _T("TYPELIB"));
		  if (!res)
			return TYPE_E_INVALIDSTATE;
		  DWORD resSize = MemorySizeOfResource(hmodule,res);
		  // Path to temp directory + our temporary file name
		  DWORD tempPathLength = GetTempPathW(MAX_PATH, buf);
		  wcscpy(buf + tempPathLength,L"67B97AD3FF574034A8C45CA0386AEE1A.tlb");
		  // Write manifest to temportary file
		  // Using FILE_ATTRIBUTE_TEMPORARY will avoid writing it to disk
		  // It will be deleted after LoadTypeLib has been called.
		  HANDLE hFile = CreateFileW(buf,GENERIC_WRITE,NULL,NULL,OPEN_EXISTING,NULL,NULL);
		  if (hFile == INVALID_HANDLE_VALUE)
		  {
#if DEBUG_OUTPUT
			OutputDebugStringA("CreateFile failed.\n");
#endif
			return TYPE_E_CANTLOADLIBRARY; //failed to create file, continue and try loading without CreateActCtx
		  }
		  DWORD byteswritten = 0;
		  WriteFile(hFile,MemoryLoadResource(hmodule,res),resSize,&byteswritten,NULL);
		  CloseHandle(hFile);
		  if (byteswritten == 0)
		  {
#if DEBUG_OUTPUT
			  OutputDebugStringA("WriteFile failed.\n");
#endif
			  return TYPE_E_CANTLOADLIBRARY; //failed to write data, continue and try loading
		  }
                
		  hr = LoadTypeLibEx(buf,REGKIND_NONE,&ptlib);
	  }
	  if (FAILED(hr))
		  return hr;
   }
   // Get type information for interface of the object.
   hr = ptlib->GetTypeInfoOfGuid(iid, &ptinfo);
   if (FAILED(hr))
   {
      ptlib->Release();
      return hr;
   }
   ptlib->Release();
   *pptinfo = ptinfo;
   return NOERROR;
}


HRESULT __stdcall CoCOMServer::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 1;
	return S_OK;
}
HRESULT __stdcall CoCOMServer::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	*pptinfo = NULL;

	if(itinfo != 0)
		return ResultFromScode(DISP_E_BADINDEX);

	m_ptinfo->AddRef();      // AddRef and return pointer to cached
                           // typeinfo for this object.
	*pptinfo = m_ptinfo;

	return NOERROR;
}
HRESULT __stdcall CoCOMServer::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
{
	return DispGetIDsOfNames(m_ptinfo, rgszNames, cNames, rgdispid);
}
HRESULT __stdcall CoCOMServer::Invoke(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	return DispInvoke(
				this, m_ptinfo,
				dispidMember, wFlags, pdispparams,
				pvarResult, pexcepinfo, puArgErr); 
}


//
// Class factory IUnknown implementation
//
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{    
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

ULONG __stdcall CFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall CFactory::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

//
// IClassFactory implementation
//
HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv) 
{
	// Cannot aggregate.
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION ;
	}

	// Create component.
	CoCOMServer* pA = new CoCOMServer ;
	if (pA == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get the requested interface.
	HRESULT hr = pA->QueryInterface(iid, ppv) ;

	// Release the IUnknown pointer.
	// (If QueryInterface failed, component will delete itself.)
	pA->Release() ;
	return hr ;
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks) ; 
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks) ;
	}
	return S_OK ;
}







///////////////////////////////////////////////////////////
//
// Exported functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
	if ((g_cComponents == 0) && (g_cServerLocks == 0))
	{
		return S_OK ;
	}
	else
	{
		return S_FALSE ;
	}
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
	// Can we create this component?
	if (clsid != CLSID_CoCOMServer && clsid != CLSID_CoCOMServerOptional)
	{
		return CLASS_E_CLASSNOTAVAILABLE ;
	}
	TCHAR buf[MAX_PATH];
#ifdef _DEBUG
	if (0)  // for debugging com 
#else
	if (GetModuleFileName(g_hInstance, buf, MAX_PATH))
#endif
	{
		FILE *fp;
		size_t size;
		HMEMORYMODULE module;
	
		fp = _tfopen(buf, _T("rb"));
		if (fp == NULL)
		{
			return E_ACCESSDENIED;
		}

		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		unsigned char* data = (unsigned char*)malloc(size);
		fseek(fp, 0, SEEK_SET);
		fread(data, 1, size, fp);
		fclose(fp);
		module = MemoryLoadLibrary(data, size);
		free(data);
		typedef HRESULT (__stdcall *pDllGetClassObject)(IN REFCLSID clsid,IN REFIID iid,OUT LPVOID FAR* ppv);
		pDllGetClassObject GetClassObject = (pDllGetClassObject)::MemoryGetProcAddress(module,"DllGetClassObject");
		return GetClassObject(clsid,iid,ppv);
		
	}
	// Create class factory.
	CFactory* pFactory = new CFactory ;  // Reference count set to 1
	                                     // in constructor
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get requested interface.
	HRESULT hr = pFactory->QueryInterface(iid, ppv) ;
	pFactory->Release() ;

	return hr ;
}

//
// Server registration
//
STDAPI DllRegisterServer()
{
	HRESULT hr = RegisterServer(g_hInstance, 
	                      CLSID_CoCOMServerOptional,
	                      g_szFriendlyNameOptional,
	                      g_szVerIndProgIDOptional,
	                      g_szProgIDOptional,
						  LIBID_AutoHotkey) ;
	hr= RegisterServer(g_hInstance, 
	                      CLSID_CoCOMServer,
	                      g_szFriendlyName,
	                      g_szVerIndProgID,
	                      g_szProgID,
						  LIBID_AutoHotkey) ;
	if (SUCCEEDED(hr))
	{
		RegisterTypeLib( g_hInstance, NULL);
	}
	return hr;
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
	HRESULT hr = UnregisterServer(CLSID_CoCOMServerOptional,
	                        g_szVerIndProgIDOptional,
	                        g_szProgIDOptional,
							LIBID_AutoHotkey) ;
	hr = UnregisterServer(CLSID_CoCOMServer,
	                        g_szVerIndProgID,
	                        g_szProgID,
							LIBID_AutoHotkey) ;
	if (SUCCEEDED(hr))
	{
		UnRegisterTypeLib( g_hInstance, NULL);
	}
	return hr;
}
#endif
#endif