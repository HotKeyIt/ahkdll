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
#include "SimpleHeap.h"
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
//#include <vld.h>

// General note:
// The use of Sleep() should be avoided *anywhere* in the code.  Instead, call MsgSleep().
// The reason for this is that if the keyboard or mouse hook is installed, a straight call
// to Sleep() will cause user keystrokes & mouse events to lag because the message pump
// (GetMessage() or PeekMessage()) is the only means by which events are ever sent to the
// hook functions.

static LPTSTR aDefaultDllScript = _T("#NoTrayIcon\n#Persistent");
static LPTSTR scriptstring = NULL;

static struct nameHinstance
     {
       HINSTANCE hInstanceP;
	   LPTSTR name ;
	   LPTSTR argv;
	   LPTSTR title;
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
		InitializeCriticalSection(&g_CriticalHeapBlocks); // used to block memory freeing in case of timeout in ahkTerminate so no corruption happens when both threads try to free Heap.
		InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
		HDC hdc = GetDC(NULL);
		g_ScreenDPI = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(NULL, hdc);
		g_HistoryTickPrev = GetTickCount();
		g_TimeLastInputPhysical = GetTickCount();
		//ahkdll(_T(""),_T(""));
#ifdef AUTODLL
	ahkdll("autoload.ahk", "");	  // used for remoteinjection of dll 
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
		 if (scriptstring)
			 free(scriptstring);
		 if (g_Debugger.mStack.mSize)
			free(g_Debugger.mStack.mBottom);
		 /*if (g_input.MatchCount)
		 {
			 free(g_input.match);
		 }*/
		 free(g_KeyHistory);
		 if (g_hWinAPI)
		 {
			 free(g_hWinAPI);
			 if (g_hWinAPIlowercase)
				free(g_hWinAPIlowercase);
		 }
		 for(int i = 0;i < FUNC_LIB_COUNT;i++)
		 {
			 if (sLib[i].length)
				 free(sLib[i].path);
		 }
		 free(g_array);
		 DeleteCriticalSection(&g_CriticalHeapBlocks); // g_CriticalHeapBlocks is used in simpleheap for thread-safety.
		 DeleteCriticalSection(&g_CriticalRegExCache); // g_CriticalRegExCache is used elsewhere for thread-safety.
		 break;
	 }
 case DLL_THREAD_DETACH:
 break;
 }

 return(TRUE); // a FALSE will abort the DLL attach
 }
 
bool ahkdll_restart_mode;
LPTSTR ahkdll_script_filespec;
TCHAR *ahkdll_param; // Small size since only numbers will be used (e.g. %1%, %2%).
bool ahkdll_switch_processing_is_complete;
int ahkdll_script_param_num;
int ahkdll_argc;
#ifndef _UNICODE
LPWSTR ahkdll_wargv;
LPWSTR *ahkdll_argv;
#else
LPWSTR *ahkdll_argv;
#endif
int ahkdll_i;
int WINAPI OldWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	OleInitialize(NULL);
	// Init any globals not in "struct g" that need it:
	g_MainThreadID = GetCurrentThreadId();
	g_script = new Script();
	g_script->Construct();
	g_clip = new Clipboard();
	g_MsgMonitor = new MsgMonitorList();
	g_SimpleHeap = new SimpleHeap();
	g_SimpleHeapVar = g_SimpleHeap;

	Object::sAnyPrototype = Object::CreateRootPrototypes();
	FileObject::sPrototype = Object::CreatePrototype(_T("File"), Object::sPrototype, FileObject::sMembers, _countof(FileObject::sMembers));
	Object::sClassPrototype = Object::CreatePrototype(_T("Class"), Object::sPrototype);
	Array::sPrototype = Object::CreatePrototype(_T("Array"), Object::sPrototype, Array::sMembers, _countof(Array::sMembers));
	Map::sPrototype = Object::CreatePrototype(_T("Map"), Object::sPrototype, Map::sMembers, _countof(Map::sMembers));
	Struct::sPrototype = Object::CreatePrototype(_T("Struct"), Object::sPrototype);
#ifdef ENABLE_DLLCALL
	DynaToken::sPrototype = Object::CreatePrototype(_T("DynaCall"), Object::sPrototype);
#endif

	UserMenu::sPrototype = Object::CreatePrototype(_T("Menu"), Object::sPrototype, UserMenu::sMembers, _countof(UserMenu::sMembers));
	UserMenu::sBarPrototype = Object::CreatePrototype(_T("MenuBar"), UserMenu::sPrototype);
	UserMenu::sClass = Object::CreateClass(_T("Menu"), Object::sClass, UserMenu::sPrototype, static_cast<ObjectMethod>(&UserMenu::New<UserMenu>));
	UserMenu::sBarClass = Object::CreateClass(_T("MenuBar"), UserMenu::sClass, UserMenu::sBarPrototype, static_cast<ObjectMethod>(&UserMenu::New<UserMenu::Bar>));

	GuiControlType::sPrototypeList = Object::CreatePrototype(_T("Gui.List"), GuiControlType::sPrototype, GuiControlType::sMembersList, _countof(GuiControlType::sMembersList));
	InputObject::sPrototype = Object::CreatePrototype(_T("InputHook"), Object::sPrototype, InputObject::sMembers, _countof(InputObject::sMembers));

	GuiType::sPrototype = Object::CreatePrototype(_T("Gui"), Object::sPrototype, GuiType::sMembers, _countof(GuiType::sMembers));
	GuiControlType::sPrototype = Object::CreatePrototype(_T("Gui.Control"), Object::sPrototype, GuiControlType::sMembers, _countof(GuiControlType::sMembers));
	GuiControlType::sPrototypeList = Object::CreatePrototype(_T("Gui.List"), GuiControlType::sPrototype, GuiControlType::sMembersList, _countof(GuiControlType::sMembersList));
	InputObject::sPrototype = Object::CreatePrototype(_T("InputHook"), Object::sPrototype, InputObject::sMembers, _countof(InputObject::sMembers));
	
	Object::sClass = Object::CreateClass(_T("Object"), Object::sClassPrototype, Object::sPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
	Object::sClassClass = Object::CreateClass(_T("Class"), Object::sClass, Object::sClassPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
	Array::sClass = Object::CreateClass(_T("Array"), Object::sClass, Array::sPrototype, static_cast<ObjectMethod>(&Array::New<Array>));
	Map::sClass = Object::CreateClass(_T("Map"), Object::sClass, Map::sPrototype, static_cast<ObjectMethod>(&Map::New<Map>));
	GuiType::sClass = Object::CreateClass(_T("Gui"), Object::sClass, GuiType::sPrototype, static_cast<ObjectMethod>(&GuiType::New<GuiType>));



	Closure::sPrototype = Object::CreatePrototype(_T("Closure"), Func::sPrototype);
	BoundFunc::sPrototype = Object::CreatePrototype(_T("BoundFunc"), Func::sPrototype);
	EnumBase::sPrototype = Object::CreatePrototype(_T("Enumerator"), Func::sPrototype);



	BufferObject::sPrototype = Object::CreatePrototype(_T("Buffer"), Object::sPrototype, BufferObject::sMembers, _countof(BufferObject::sMembers));
	ClipboardAll::sPrototype = Object::CreatePrototype(_T("ClipboardAll"), BufferObject::sPrototype);

	RegExMatchObject::sPrototype = Object::CreatePrototype(_T("RegExMatch"), Object::sPrototype, RegExMatchObject::sMembers, _countof(RegExMatchObject::sMembers));

	Object::sPrimitivePrototype = Object::CreatePrototype(_T("Primitive"), Object::sAnyPrototype);
	Object::sStringPrototype = Object::CreatePrototype(_T("String"), Object::sPrimitivePrototype);
	Object::sNumberPrototype = Object::CreatePrototype(_T("Number"), Object::sPrimitivePrototype);
	Object::sIntegerPrototype = Object::CreatePrototype(_T("Integer"), Object::sNumberPrototype);
	Object::sFloatPrototype = Object::CreatePrototype(_T("Float"), Object::sNumberPrototype);

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

	// v1.1.22+: This is done unconditionally, on startup, so that any attempts to read a drive
	// that has no media (and possibly other errors) won't cause the system to display an error
	// dialog that the script can't suppress.  This is known to affect floppy drives and some
	// but not all CD/DVD drives.  MSDN says: "Best practice is that all applications call the
	// process-wide SetErrorMode function with a parameter of SEM_FAILCRITICALERRORS at startup."
	// Note that in previous versions, this was done by the Drive/DriveGet commands and not
	// reverted afterward, so it affected all subsequent commands.
	SetErrorMode(SEM_FAILCRITICALERRORS);

	UpdateWorkingDir(); // Needed for the FileSelect() workaround.
	g_WorkingDirOrig = g_SimpleHeap->Malloc(const_cast<LPTSTR>(g_WorkingDir.GetString())); // Needed by the Reload command.

	// Set defaults, to be overridden by command line args we receive:
	ahkdll_restart_mode = false;

	ahkdll_script_filespec = lpCmdLine ; // Naveen changed from NULL;

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

	ahkdll_switch_processing_is_complete = false;
	ahkdll_script_param_num = 1;

	ahkdll_argc = 0;
#ifndef _UNICODE
	ahkdll_wargv = (LPWSTR) _alloca((_tcslen(nameHinstanceP.argv)+1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8,0,nameHinstanceP.argv,-1,wargv,(_tcslen(nameHinstanceP.argv)+1)*sizeof(WCHAR));
	ahkdll_argv = CommandLineToArgvW(wargv,&dllargc);
#else
	ahkdll_argv = CommandLineToArgvW(nameHinstanceP.argv,&ahkdll_argc);
#endif
	ahkdll_i = 0;
	if (*nameHinstanceP.argv) // Only process if parameters were given
		for (ahkdll_i = 0; ahkdll_i < ahkdll_argc; ++ahkdll_i) // Start at 0 because 0 does not contains the program name or script.
		{
#ifndef _UNICODE
			param = (TCHAR *) _alloca((wcslen(dllargv[i])+1)*sizeof(CHAR));
			WideCharToMultiByte(CP_ACP,0,wargv,-1,param,(wcslen(dllargv[i])+1)*sizeof(CHAR),0,0);
#else
			ahkdll_param = ahkdll_argv[ahkdll_i]; // For performance and convenience.
#endif
			//if (switch_processing_is_complete) // All args are now considered to be input parameters for the script.
			//{
			//	if (   !(var = g_script->FindOrAddVar(var_name, _stprintf(var_name, _T("%d"), script_param_num)))   )
			//		return CRITICAL_ERROR;  // Realistically should never happen.
			//	var->Assign(param);
			//	++script_param_num;
			//}

			// Insist that switches be an exact match for the allowed values to cut down on ambiguity.
			// For example, if the user runs "CompiledScript.exe /find", we want /find to be considered
			// an input parameter for the script rather than a switch:
			if (!_tcsicmp(ahkdll_param, _T("/R")) || !_tcsicmp(ahkdll_param, _T("/restart")))
				ahkdll_restart_mode = true;
			else if (!_tcsicmp(ahkdll_param, _T("/F")) || !_tcsicmp(ahkdll_param, _T("/force")))
				g_ForceLaunch = true;
			else if (!_tcsicmp(ahkdll_param, _T("/ErrorStdOut")))
				g_script->SetErrorStdOut(ahkdll_param[12] == '=' ? ahkdll_param + 13 : NULL);
			else if (!_tcsicmp(ahkdll_param, _T("/iLib"))) // v1.0.47: Build an include-file so that ahk2exe can include library functions called by the script.
			{
				++ahkdll_i; // Consume the next parameter too, because it's associated with this one.
				if (ahkdll_i >= ahkdll_argc) // Missing the expected filename parameter.
				{
					g_Reloading = false;
					return CRITICAL_ERROR;
				}
				// For performance and simplicity, open/create the file unconditionally and keep it open until exit.
				g_script->mIncludeLibraryFunctionsThenExit = new TextFile;
				if (!g_script->mIncludeLibraryFunctionsThenExit->Open(ahkdll_argv[ahkdll_i], TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
				{
					g_Reloading = false;
					return CRITICAL_ERROR;
				}
			}
			else if (!_tcsicmp(ahkdll_param, _T("/E")) || !_tcsicmp(ahkdll_param, _T("/Execute")))
			{
				g_hResource = NULL; // Execute script from File. Override compiled, A_IsCompiled will also report 0
			}
			else if (!_tcsnicmp(ahkdll_param, _T("/CP"), 3)) // /CPnnn
			{
				// Default codepage for the script file, NOT the default for commands used by it.
				g_DefaultScriptCodepage = ATOU(ahkdll_param + 3);
			}
#ifdef CONFIG_DEBUGGER
			// Allow a debug session to be initiated by command-line.
			else if (!g_Debugger.IsConnected() && !_tcsnicmp(ahkdll_param, _T("/Debug"), 6) && (ahkdll_param[6] == '\0' || ahkdll_param[6] == '='))
			{
				if (ahkdll_param[6] == '=')
				{
					ahkdll_param += 7;

					LPTSTR c = _tcsrchr(ahkdll_param, ':');

					if (c)
					{
						StringTCharToChar(ahkdll_param, g_DebuggerHost, (int)(c - ahkdll_param));
						StringTCharToChar(c + 1, g_DebuggerPort);
					}
					else
					{
						StringTCharToChar(ahkdll_param, g_DebuggerHost);
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
				break;  // No more switches allowed after this point.
			}
		}

	if (Var *var = g_script->FindOrAddVar(_T("A_Args"), 6, VAR_DECLARE_SUPER_GLOBAL))
	{
		// Store the remaining args in an array and assign it to "A_Args".
		// If there are no args, assign an empty array so that A_Args[1]
		// and A_Args.Length() don't cause an error.
		auto *args = Array::FromArgV((LPTSTR*)(ahkdll_argv + ahkdll_i), ahkdll_argc - ahkdll_i);
		if (!args)
		{
			g_Reloading = false;
			return CRITICAL_ERROR;  // Realistically should never happen.
		}
		var->AssignSkipAddRef(args);
	}
	else
	{
		g_Reloading = false;
		return CRITICAL_ERROR;
	}
	LocalFree(ahkdll_argv); // free memory allocated by CommandLineToArgvW

	global_init(*g);  // Set defaults prior to the below, since below might override them for AutoIt2 scripts.

	// Set up the basics of the script:
	if (g_script->Init(*g, ahkdll_script_filespec, ahkdll_restart_mode,hInstance,g_hResource ? 0 : (bool)nameHinstanceP.istext) != OK)  // Set up the basics of the script, using the above.
	{
		g_Reloading = false;
		return CRITICAL_ERROR;
	}

	//if (nameHinstanceP.istext)
	//	GetCurrentDirectory(MAX_PATH, g_script->mFileDir);
	// Could use CreateMutex() but that seems pointless because we have to discover the
	// hWnd of the existing process so that we can close or restart it, so we would have
	// to do this check anyway, which serves both purposes.  Alt method is this:
	// Even if a 2nd instance is run with the /force switch and then a 3rd instance
	// is run without it, that 3rd instance should still be blocked because the
	// second created a 2nd handle to the mutex that won't be closed until the 2nd
	// instance terminates, so it should work ok:
	//CreateMutex(NULL, FALSE, script_filespec); // script_filespec seems a good choice for uniqueness.
	//if (!g_ForceLaunch && !restart_mode && GetLastError() == ERROR_ALREADY_EXISTS)

	LineNumberType load_result = (g_hResource || !nameHinstanceP.istext) ? g_script->LoadFromFile() : g_script->LoadFromText(ahkdll_script_filespec);
	if (load_result == LOADING_FAILED) // Error during load (was already displayed by the function call).
	{
		g_Reloading = false;
		if (g_script->mIncludeLibraryFunctionsThenExit)
			g_script->mIncludeLibraryFunctionsThenExit->Close(); // Flush its buffer to disk.
		return CRITICAL_ERROR;  // Should return this value because PostQuitMessage() also uses it.
	}
	if (!load_result) // LoadFromFile() relies upon us to do this check.  No lines were loaded, so we're done.
	{
		g_Reloading = false;
		return 0;
	}

	if (*nameHinstanceP.title)
		g_script->mFileName = nameHinstanceP.title;
	// Create all our windows and the tray icon.  This is done after all other chances
	// to return early due to an error have passed, above.
	if (g_script->CreateWindows() != OK)
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
	
	if (g_MaxHistoryKeys && (g_KeyHistory = (KeyHistoryItem *)realloc(g_KeyHistory,g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
		ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
	//else leave it NULL as it was initialized in globaldata.

#ifdef CONFIG_DEBUGGER
	// Initiate debug session now if applicable.
	if (!g_DebuggerHost.IsEmpty() && g_Debugger.Connect(g_DebuggerHost, g_DebuggerPort) == DEBUGGER_E_OK)
	{
		g_Debugger.Break();
	}
#endif
	/* // Use ExceptionHandler only in main process
	// set exception filter to disable hook before exception occures to avoid system/mouse freeze
	// also when dll will crash, it will only exit dll thread and try to destroy it, leaving the exe process running
	// specify 1 so dll handler runs before exe handler
	g_ExceptionHandler = AddVectoredExceptionHandler(1,DisableHooksOnException);
	*/

	// set exception filter to disable hook before exception occures to avoid system/mouse freeze
	g_ExceptionHandler = AddVectoredExceptionHandler(NULL,DisableHooksOnException);

	// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
	// top part (the auto-execute part) of the script so that they will be in effect even if the
	// top part is something that's very involved and requires user interaction:
	Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
	//Hotkey::InstallKeybdHook();
	//Hotkey::InstallMouseHook();
	//if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
	//	AddRemoveHooks(3);
	g_script->mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
	//Sleep(20);
	g_Reloading = false;
	//free(nameHinstanceP.name);
	
	// Run the auto-execute part at the top of the script (this call might never return):
	if (!g_script->AutoExecSection()) // Can't run script at all. Due to rarity, just abort.
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

EXPORT int ahkTerminate(int timeout = 0)
{
	DWORD lpExitCode = 0;
	if (g_hThread == 0)
		return 0;
	g_AllowInterruption = FALSE;
	GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
	DWORD tickstart = GetTickCount();
	DWORD timetowait = timeout < 0 ? timeout * -1 : timeout;
	SendMessageTimeout(g_hWnd, AHK_EXIT_BY_SINGLEINSTANCE, OK, 0,timeout < 0 ? SMTO_NORMAL : SMTO_NOTIMEOUTIFNOTHUNG,SLEEP_INTERVAL * 3,0);
	for (; g_hThread && g_script && g_script->mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259) && (timeout == 0 || timetowait > (GetTickCount()-tickstart));)
	{
		Sleep(100); // give it a bit time to exit thread
	}
	if ((g_script && g_script->mIsReadyToExecute) || g_hThread)
	{
		SuspendThread(g_hThread);
		delete g_clip;
		g_clip = NULL;
		//delete g_script; // disabled as it migth cause a crash
		g_script = NULL;
		CloseHandle(g_hThread);
		TerminateThread(g_hThread, (DWORD)EARLY_EXIT);
		g_hThread = NULL;
	}
	g_AllowInterruption = TRUE;
	return 0;
}

// Naveen: v1. runscript() - runs the script in a separate thread compared to host application.
unsigned __stdcall runScript( void* pArguments )
{
	int result = OldWinMain(nameHinstanceP.hInstanceP, 0, nameHinstanceP.name, 0);
	delete g_script;
	g_script = NULL;
	delete g_clip;
	g_clip = NULL;
	g_hThread = NULL;
	_endthreadex( result );  
    return 0;
}


void WaitIsReadyToExecute()
{
	 int lpExitCode = 0;
	 while ((!g_script && g_hThread) || (g_hThread && !g_script->mIsReadyToExecute && (lpExitCode == 0 || lpExitCode == 259)))
	 {
		Sleep(10);
		GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
	 }
	 if (g_hThread && (!g_script || !g_script->mIsReadyToExecute))
	 {
		CloseHandle(g_hThread);
		g_hThread = NULL;
		SetLastError(lpExitCode);
	 }
}


HANDLE runThread()
{
	if (g_hThread)
	{	// Small check to be done to make sure we do not start a new thread before the old is closed
		Sleep(50); // make sure script is not in starting state
		int lpExitCode = 0;
		GetExitCodeThread(g_hThread,(LPDWORD)&lpExitCode);
		if ((lpExitCode == 0 || lpExitCode == 259) && (g_script && g_script->mIsReadyToExecute))
			Sleep(50); // make sure the script is not about to be terminated, because this might lead to problems
		if (g_hThread && (g_script && g_script->mIsReadyToExecute))
 			ahkTerminate(0);
	}
	g_hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, NULL, 0, 0 );
	WaitIsReadyToExecute();
	return g_hThread;
}

int setscriptstrings(LPTSTR fileName, LPTSTR argv, LPTSTR title)
{
	LPTSTR newstring = (LPTSTR)realloc(scriptstring,(_tcslen(fileName) + _tcslen(argv) + _tcslen(title) + 3) * sizeof(TCHAR));
	if (!newstring)
		return 1;
	memset(newstring, 0, (_tcslen(fileName) + _tcslen(argv) + _tcslen(title) + 3) * sizeof(TCHAR));
	scriptstring = newstring;
	_tcscpy(scriptstring,fileName);
	_tcscpy(scriptstring + _tcslen(fileName) + 1,argv);
	_tcscpy(scriptstring + _tcslen(fileName) + _tcslen(argv) + 2,title);
	nameHinstanceP.name = scriptstring;
	nameHinstanceP.argv = scriptstring + _tcslen(fileName) + 1 ;
	nameHinstanceP.title = scriptstring + _tcslen(fileName) + _tcslen(argv) + 2 ;
	return 0;
}

EXPORT HANDLE ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR title)
{
	if (setscriptstrings(fileName && !IsBadReadPtr(fileName,1) && *fileName ? fileName : aDefaultDllScript, argv && !IsBadReadPtr(argv,1) && *argv ? argv : _T(""), title && !IsBadReadPtr(title,1) && *title ? title : _T("")))
		return 0;
	nameHinstanceP.istext = *fileName ? 0 : 1;
	return runThread();
}

// HotKeyIt ahktextdll
EXPORT HANDLE ahktextdll(LPTSTR fileName, LPTSTR argv, LPTSTR title)
{
	if (setscriptstrings(fileName && !IsBadReadPtr(fileName,1) && *fileName ? fileName : aDefaultDllScript, argv && !IsBadReadPtr(argv,1) && *argv ? argv : _T(""), title && !IsBadReadPtr(title,1) && *title ? title : _T("")))
		return 0;
	nameHinstanceP.istext = 1;
	return runThread();
}

void reloadDll()
{
	delete g_script;
	g_script = NULL;
	delete g_clip;
	g_clip = NULL;
	g_hThread = (HANDLE)_beginthreadex( NULL, 0, &runScript, &nameHinstanceP, 0, 0 );
	g_AllowInterruption = TRUE;
	_endthreadex( (DWORD)EARLY_RETURN );
}

ResultType terminateDll(int aExitCode)
{
	delete g_script;
	g_script = NULL;
	delete g_clip;
	g_clip = NULL;
	g_AllowInterruption = TRUE;
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


// COM Implementation //
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

// Friendly name of component
const char g_szFriendlyName[] = "AutoHotkey2 Script" ;

// Version-independent ProgID
const char g_szVerIndProgID[] = "AutoHotkey2.Script" ;

// ProgID
const char g_szProgID[] = "AutoHotkey2.Script.1" ;

#ifdef _WIN64
const char g_szFriendlyNameOptional[] = "AutoHotkey2 Script X64" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey2.Script.X64" ;
const char g_szProgIDOptional[] = "AutoHotkey2.Script.X64.1" ;
#else
#ifdef _UNICODE
const char g_szFriendlyNameOptional[] = "AutoHotkey2 Script UNICODE" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey2.Script.UNICODE" ;
const char g_szProgIDOptional[] = "AutoHotkey2.Script.UNICODE.1" ;
#else
const char g_szFriendlyNameOptional[] = "AutoHotkey2 Script ANSI" ;
const char g_szVerIndProgIDOptional[] = "AutoHotkey2.Script.ANSI" ;
const char g_szProgIDOptional[] = "AutoHotkey2.Script.ANSI.1" ;
#endif // UNICODE
#endif // WIN64

//
// Constructor
//
CoCOMServer::CoCOMServer() : m_cRef(1)

{ 
	InterlockedIncrement(&g_cComponents) ; 

	m_ptinfo = NULL;
	LoadTypeInfo(&m_ptinfo, LIBID_AutoHotkey2, IID_ICOMServer, 0);
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

HRESULT __stdcall CoCOMServer::ahktextdll(/*in,optional*/VARIANT script,/*in,optional*/VARIANT params,/*in,optional*/VARIANT title,/*out*/UINT_PTR* hThread)
{
	USES_CONVERSION;
	TCHAR buf1[MAX_INTEGER_SIZE],buf2[MAX_INTEGER_SIZE];
	if (hThread==NULL)
		return ERROR_INVALID_PARAMETER;
	*hThread = (UINT_PTR)com_ahktextdll(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script,buf1)
							,params.vt == VT_BSTR ? OLE2T(params.bstrVal) : Variant2T(params,buf2)
							,title.vt == VT_BSTR ? OLE2T(title.bstrVal) : Variant2T(title,buf2));
	return S_OK;
}

HRESULT __stdcall CoCOMServer::ahkdll(/*in,optional*/VARIANT filepath,/*in,optional*/VARIANT params,/*in,optional*/VARIANT title,/*out*/UINT_PTR* hThread)
{
	USES_CONVERSION;
	TCHAR buf1[MAX_INTEGER_SIZE],buf2[MAX_INTEGER_SIZE];
	if (hThread==NULL)
		return ERROR_INVALID_PARAMETER;
	*hThread = (UINT_PTR)com_ahkdll(filepath.vt == VT_BSTR ? OLE2T(filepath.bstrVal) : Variant2T(filepath,buf1)
							,params.vt == VT_BSTR ? OLE2T(params.bstrVal) : Variant2T(params,buf2)
							,title.vt == VT_BSTR ? OLE2T(title.bstrVal) : Variant2T(title,buf2));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkPause(/*in,optional*/VARIANT aChangeTo,/*out*/int* paused)
{
	USES_CONVERSION;
	if (paused==NULL)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*paused = com_ahkPause(aChangeTo.vt == VT_BSTR ? OLE2T(aChangeTo.bstrVal) : Variant2T(aChangeTo,buf));
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkReady(/*out*/int* ready)
{
	if (ready==NULL)
		return ERROR_INVALID_PARAMETER;
	*ready = com_ahkReady();
	return S_OK;
}
HRESULT __stdcall CoCOMServer::ahkIsUnicode(/*out*/int* IsUnicode)
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
	
	var = g_script->FindVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name,buf)) ;
	var->ToToken(aToken) ;
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
   if (   !(var = g_script->FindOrAddVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name,namebuf)))   )
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
HRESULT __stdcall CoCOMServer::ahkLabel(/*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ int* success)
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
		  wcscpy(buf + tempPathLength,L"104298D07DAE41B08F867A3EC39903AC.tlb");
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
						  LIBID_AutoHotkey2) ;
	hr= RegisterServer(g_hInstance, 
	                      CLSID_CoCOMServer,
	                      g_szFriendlyName,
	                      g_szVerIndProgID,
	                      g_szProgID,
						  LIBID_AutoHotkey2) ;
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
							LIBID_AutoHotkey2) ;
	hr = UnregisterServer(CLSID_CoCOMServer,
	                        g_szVerIndProgID,
	                        g_szProgID,
							LIBID_AutoHotkey2) ;
	if (SUCCEEDED(hr))
	{
		UnRegisterTypeLib( g_hInstance, NULL);
	}
	return hr;
}
#endif