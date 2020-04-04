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
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "window.h" // For MsgBox() & SetForegroundLockTimeout()
#include "TextIO.h"
#include "LiteZip.h"
#include <process.h>
//#include <vld.h> // find memory leaks

#ifndef _USRDLL

BOOL g_TlsDoExecute = false;
DWORD g_TlsOldProtect;
#ifdef _M_IX86 // compiles for x86
#pragma comment(linker,"/include:__tls_used") // This will cause the linker to create the TLS directory
#elif _M_AMD64 // compiles for x64
#pragma comment(linker,"/include:_tls_used") // This will cause the linker to create the TLS directory
#endif
#pragma section(".CRT$XLB",read) // Create a new section

typedef LONG(NTAPI *MyNtQueryInformationProcess)(HANDLE hProcess, ULONG InfoClass, PVOID Buffer, ULONG Length, PULONG ReturnLength);
typedef LONG(NTAPI *MyNtSetInformationThread)(HANDLE ThreadHandle, ULONG ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
#define NtCurrentProcess() (HANDLE)-1

// The TLS callback is called before the process entry point executes, and is executed before the debugger breaks
// This allows you to perform anti-debugging checks before the debugger can do anything
// Therefore, TLS callback is a very powerful anti-debugging technique

void WINAPI TlsCallback(PVOID Module, DWORD Reason, PVOID Context)
{
	HANDLE DebugPort = NULL;
	
	// Execute only if A_IsCompiled
#ifdef _DEBUG
	g_TlsDoExecute = true;
	return;
#endif
#ifndef _DEBUG
	PBOOLEAN BeingDebugged;
	if (!FindResource(NULL, _T("E4847ED08866458F8DD35F94B37001C0"), RT_RCDATA))
	{
		g_TlsDoExecute = true;
		return;
	}
	Sleep(20);
#ifdef _M_IX86 // compiles for x86
	BeingDebugged = (PBOOLEAN)__readfsdword(0x30) + 2;
#elif _M_AMD64 // compiles for x64
	BeingDebugged = (PBOOLEAN)__readgsqword(0x60) + 2; //0x60 because offset is doubled in 64bit
#endif
	if (*BeingDebugged) // Read the PEB
		return;
	char filename[MAX_PATH];
	HFILE fp;
	HMODULE hModule = GetModuleHandleA("ntdll.dll");
	unsigned char* data = (unsigned char*)GlobalAlloc(NULL, 0x300000); // 3MB should be sufficient
	GetModuleFileNameA(hModule, filename, MAX_PATH);
	g_hNTDLL = MemoryLoadLibrary(data, _lread(fp = _lopen(filename, OF_READ), data, 0x300000), false);
	_lclose(fp);
	GlobalFree(data);
	MyNtQueryInformationProcess _NtQueryInformationProcess = (MyNtQueryInformationProcess)MemoryGetProcAddress(g_hNTDLL, "NtQueryInformationProcess");
	if (!_NtQueryInformationProcess(NtCurrentProcess(), 7, &DebugPort, sizeof(HANDLE), NULL) && DebugPort)
		return;
#ifdef _WIN64
	MyNtSetInformationThread _NtSetInformationThread = (MyNtSetInformationThread)MemoryGetProcAddress(g_hNTDLL, "NtSetInformationThread");
#else
	MyNtSetInformationThread _NtSetInformationThread = (MyNtSetInformationThread)GetProcAddress(hModule, "NtSetInformationThread");
#endif
	_NtSetInformationThread(GetCurrentThread(), 0x11, 0, 0);
	BOOL _BeingDebugged;
	CheckRemoteDebuggerPresent(GetCurrentProcess(), &_BeingDebugged);
	if (_BeingDebugged)
		return;
	FILETIME SystemTime, CreationTime = { 0 };
	FILETIME ExitTime, KernelTime, UserTime;
	GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime);
	GetSystemTimeAsFileTime(&SystemTime);
	TCHAR timerbuf[MAX_INTEGER_LENGTH];
	_i64tot((((((ULONGLONG)SystemTime.dwHighDateTime) << 32) + SystemTime.dwLowDateTime) - ((((ULONGLONG)CreationTime.dwHighDateTime) << 32) + CreationTime.dwLowDateTime)), timerbuf, 10);
	ULONGLONG time = ((((((ULONGLONG)SystemTime.dwHighDateTime) << 32) + SystemTime.dwLowDateTime) - ((((ULONGLONG)CreationTime.dwHighDateTime) << 32) + CreationTime.dwLowDateTime)));
	if (time > 20000000 || time < 1000)
		return;
	hModule = GetModuleHandleA("kernel32.dll");
	data = (unsigned char*)GlobalAlloc(NULL, 0x300000); // 3MB should be sufficient
	GetModuleFileNameA(hModule, filename, MAX_PATH);
	g_hKERNEL32 = MemoryLoadLibrary(data, _lread(fp = _lopen(filename, OF_READ), data, 0x300000), false);
	_lclose(fp);
	GlobalFree(data);
	((_QueryPerformanceCounter)MemoryGetProcAddress(g_hKERNEL32, "QueryPerformanceFrequency"))((LARGE_INTEGER*)&g_QPCfreq);
	(g_QPC = (_QueryPerformanceCounter)MemoryGetProcAddress(g_hKERNEL32, "QueryPerformanceCounter"))((LARGE_INTEGER*)&g_QPCtimer);
	g_TlsDoExecute = true;
#endif
}
void WINAPI TlsCallbackCall(PVOID Module, DWORD Reason, PVOID Context);
__declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK CallbackAddress[] = { TlsCallbackCall, NULL, TlsCallback }; // Put the TLS callback address into a null terminated array of the .CRT$XLB section
void WINAPI TlsCallbackCall(PVOID Module, DWORD Reason, PVOID Context)
{
	VirtualProtect(CallbackAddress + sizeof(UINT_PTR), sizeof(UINT_PTR), PAGE_EXECUTE_READWRITE, &g_TlsOldProtect);
	for (int i = 0; i < sizeof(UINT_PTR); i++)
		*((BYTE*)&CallbackAddress[1] + i) = ((BYTE*)&CallbackAddress[2])[i];
	CallbackAddress[2] = NULL;
	VirtualProtect(CallbackAddress + sizeof(UINT_PTR), sizeof(UINT_PTR), g_TlsOldProtect, &g_TlsOldProtect);
}


// The entry point is executed after the TLS callback

// General note:
// The use of Sleep() should be avoided *anywhere* in the code.  Instead, call MsgSleep().
// The reason for this is that if the keyboard or mouse hook is installed, a straight call
// to Sleep() will cause user keystrokes & mouse events to lag because the message pump
// (GetMessage() or PeekMessage()) is the only means by which events are ever sent to the
// hook functions.
#include <iphlpapi.h>
int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// Init any globals not in "struct g" that need it:
#ifdef _DEBUG
	g_hResource = FindResource(NULL, _T("AHK"), RT_RCDATA);
#else
	g_hResource = FindResource(NULL, _T("E4847ED08866458F8DD35F94B37001C0"), RT_RCDATA);
#endif
	g_hInstance = hInstance;
	g_HistoryTickPrev = GetTickCount();
	g_TimeLastInputPhysical = GetTickCount();
	g_script = new Script();
	g_script->Construct();
	g_clip = new Clipboard();
	g_MsgMonitor = new MsgMonitorList();
	g_SimpleHeap = new SimpleHeap();
	g_SimpleHeapVar = g_SimpleHeap;
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
	InitializeCriticalSection(&g_CriticalAhkFunction); // used to call a function in multithreading environment.

	InitializeCriticalSection(&g_CriticalDebugger);
	g_ahkThreads[0][1] = (UINT_PTR)g_script;
	g_ahkThreads[0][4] = (UINT_PTR)&g_startup;
	g_ahkThreads[0][5] = (UINT_PTR)g_ThreadID;
	g_ahkThreads[0][6] = (UINT_PTR)((PMYTEB)NtCurrentTeb())->ThreadLocalStoragePointer;

	Object::sAnyPrototype = Object::CreateRootPrototypes();
	FileObject::sPrototype = Object::CreatePrototype(_T("File"), Object::sPrototype, FileObject::sMembers, _countof(FileObject::sMembers));
	Object::sClassPrototype = Object::CreatePrototype(_T("Class"), Object::sPrototype);
	Array::sPrototype = Object::CreatePrototype(_T("Array"), Object::sPrototype, Array::sMembers, _countof(Array::sMembers));
	Map::sPrototype = Object::CreatePrototype(_T("Map"), Object::sPrototype, Map::sMembers, _countof(Map::sMembers));
	Struct::sPrototype = Object::CreatePrototype(_T("Struct"), Object::sPrototype);
#ifdef ENABLE_DLLCALL
	DynaToken::sPrototype = Object::CreatePrototype(_T("DynaCall"), Object::sPrototype);
#endif

	UserMenu::sMenuPrototype = Object::CreatePrototype(_T("Menu"), Object::sPrototype, UserMenu::sMembers, _countof(UserMenu::sMembers));
	UserMenu::sMenuBarPrototype = Object::CreatePrototype(_T("MenuBar"), UserMenu::sMenuPrototype);

	GuiType::sPrototype = Object::CreatePrototype(_T("Gui"), Object::sPrototype, GuiType::sMembers, _countof(GuiType::sMembers));
	GuiControlType::sPrototype = Object::CreatePrototype(_T("Gui.Control"), Object::sPrototype, GuiControlType::sMembers, _countof(GuiControlType::sMembers));
	GuiControlType::sPrototypeList = Object::CreatePrototype(_T("Gui.List"), GuiControlType::sPrototype, GuiControlType::sMembersList, _countof(GuiControlType::sMembersList));
	InputObject::sPrototype = Object::CreatePrototype(_T("InputHook"), Object::sPrototype, InputObject::sMembers, _countof(InputObject::sMembers));

	Object::sClass = Object::CreateClass(_T("Object"), Object::sClassPrototype, Object::sPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
	Object::sClassClass = Object::CreateClass(_T("Class"), Object::sClass, Object::sClassPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
	Array::sClass = Object::CreateClass(_T("Array"), Object::sClass, Array::sPrototype, static_cast<ObjectMethod>(&Array::New<Array>));
	Map::sClass = Object::CreateClass(_T("Map"), Object::sClass, Map::sPrototype, static_cast<ObjectMethod>(&Map::New<Map>));



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
	bool restart_mode = false;
	#ifdef _DEBUG				// HotKeyIt, debugger will launch AutoHotkey.ahk as normal AutoHotkey.exe does
		TCHAR *script_filespec = _T("Test\\Test.ahk");
	#else
		TCHAR *script_filespec = NULL; // Set default as "unspecified/omitted".
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
	int i;
	for (i = 1; i < __argc; ++i) // Start at 1 because 0 contains the program name.
	{
		LPTSTR param = __targv[i]; // For performance and convenience.
		// Insist that switches be an exact match for the allowed values to cut down on ambiguity.
		// For example, if the user runs "CompiledScript.exe /find", we want /find to be considered
		// an input parameter for the script rather than a switch:
		if (!_tcsicmp(param, _T("/R")) || !_tcsicmp(param, _T("/restart")))
			restart_mode = true;
		else if (!_tcsicmp(param, _T("/F")) || !_tcsicmp(param, _T("/force")))
			g_ForceLaunch = true;
		else if (!_tcsicmp(param, _T("/ErrorStdOut")))
			g_script->mErrorStdOut = true;
		else if (!_tcsicmp(param, _T("/iLib"))) // v1.0.47: Build an include-file so that ahk2exe can include library functions called by the script.
		{
			++i; // Consume the next parameter too, because it's associated with this one.
			if (i >= __argc) // Missing the expected filename parameter.
				return CRITICAL_ERROR;
			// For performance and simplicity, open/create the file unconditionally and keep it open until exit.
			g_script->mIncludeLibraryFunctionsThenExit = new TextFile;
			if (!g_script->mIncludeLibraryFunctionsThenExit->Open(__targv[i], TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
				return CRITICAL_ERROR;
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
			if (!g_hResource) // Only apply if it is not a compiled AutoHotkey.exe
			{
				script_filespec = param;  // The first unrecognized switch must be the script filespec, by design.
				++i; // Omit this from the "args" array.
			}
			break; // No more switches allowed after this point.
		}
	}

	if (Var *var = g_script->FindOrAddVar(_T("A_Args"), 6, VAR_DECLARE_SUPER_GLOBAL))
	{
		// Store the remaining args in an array and assign it to "A_Args".
		// If there are no args, assign an empty array so that A_Args[1]
		// and A_Args.Length() don't cause an error.
		auto args = Array::FromArgV(__targv + i, __argc - i);
		if (!args)
			return CRITICAL_ERROR;  // Realistically should never happen.
		var->AssignSkipAddRef(args);
	}
	else
		return CRITICAL_ERROR;

	global_init(*g);  // Set defaults.
	
	if (!g_TlsDoExecute)
		return 0;

	HMODULE advapi32 = LoadLibrary(_T("advapi32.dll"));
	g_CryptEncrypt = (MyCryptEncrypt)GetProcAddress(advapi32, "CryptEncrypt");
	g_CryptDecrypt = (MyCryptDecrypt)GetProcAddress(advapi32, "CryptDecrypt");
// Set up the basics of the script:
	if (g_script->Init(*g, script_filespec, restart_mode,0,false) != OK)  // Set up the basics of the script, using the above.
		return CRITICAL_ERROR;
	// Set g_default now, reflecting any changes made to "g" above, in case AutoExecSection(), below,
	// never returns, perhaps because it contains an infinite loop (intentional or not):
	CopyMemory(&g_default, g, sizeof(global_struct));

	// Use FindOrAdd vs Add for maintainability, although it shouldn't already exist:
	if (   !(g_ErrorLevel = g_script->FindOrAddVar(_T("ErrorLevel")))   )
		return CRITICAL_ERROR; // Error.  Above already displayed it for us.
	// Initialize the var state to zero:
	g_ErrorLevel->Assign(ERRORLEVEL_NONE);

	// Could use CreateMutex() but that seems pointless because we have to discover the
	// hWnd of the existing process so that we can close or restart it, so we would have
	// to do this check anyway, which serves both purposes.  Alt method is this:
	// Even if a 2nd instance is run with the /force switch and then a 3rd instance
	// is run without it, that 3rd instance should still be blocked because the
	// second created a 2nd handle to the mutex that won't be closed until the 2nd
	// instance terminates, so it should work ok:
	//CreateMutex(NULL, FALSE, script_filespec); // script_filespec seems a good choice for uniqueness.
	//if (!g_ForceLaunch && !restart_mode && GetLastError() == ERROR_ALREADY_EXISTS)

	UINT load_result = g_script->LoadFromFile();
	if (load_result == LOADING_FAILED) // Error during load (was already displayed by the function call).
	{
		if (g_script->mIncludeLibraryFunctionsThenExit)
			g_script->mIncludeLibraryFunctionsThenExit->Close(); // Flush its buffer to disk.
		return CRITICAL_ERROR;  // Should return this value because PostQuitMessage() also uses it.
	}
	if (!load_result) // LoadFromFile() relies upon us to do this check.  No script was loaded or we're in /iLib mode, so nothing more to do.
		return 0;
	g_ahkThreads[0][2] = (UINT_PTR)Line::sSourceFile;
	g_ahkThreads[0][3] = (UINT_PTR)Line::sSourceFileCount;
	HWND w_existing = NULL;
	UserMessages reason_to_close_prior = (UserMessages)0;
	if (g_AllowOnlyOneInstance && !restart_mode && !g_ForceLaunch)
	{
		// Note: the title below must be constructed the same was as is done by our
		// CreateWindows(), which is why it's standardized in g_script->mMainWindowTitle:
		if (w_existing = FindWindow(WINDOW_CLASS_MAIN, g_script->mMainWindowTitle))
		{
			if (g_AllowOnlyOneInstance == SINGLE_INSTANCE_IGNORE)
				return 0;
			if (g_AllowOnlyOneInstance != SINGLE_INSTANCE_REPLACE)
				if (MsgBox(_T("An older instance of this script is already running.  Replace it with this")
					_T(" instance?\nNote: To avoid this message, see #SingleInstance in the help file.")
					, MB_YESNO, g_script->mFileName) == IDNO)
					return 0;
			// Otherwise:
			reason_to_close_prior = AHK_EXIT_BY_SINGLEINSTANCE;
		}
	}
	if (!reason_to_close_prior && restart_mode)
		if (w_existing = FindWindow(WINDOW_CLASS_MAIN, g_script->mMainWindowTitle))
			reason_to_close_prior = AHK_EXIT_BY_RELOAD;
	if (reason_to_close_prior)
	{
		// Now that the script has been validated and is ready to run, close the prior instance.
		// We wait until now to do this so that the prior instance's "restart" hotkey will still
		// be available to use again after the user has fixed the script.  UPDATE: We now inform
		// the prior instance of why it is being asked to close so that it can make that reason
		// available to the OnExit function via a built-in variable:
		ASK_INSTANCE_TO_CLOSE(w_existing, reason_to_close_prior);
		//PostMessage(w_existing, WM_CLOSE, 0, 0);

		// Wait for it to close before we continue, so that it will deinstall any
		// hooks and unregister any hotkeys it has:
		int interval_count;
		for (interval_count = 0; ; ++interval_count)
		{
			Sleep(50);  // No need to use MsgSleep() in this case.
			if (!IsWindow(w_existing))
				break;  // done waiting.
			if (interval_count == 100)
			{
				// This can happen if the previous instance has an OnExit function that takes a long
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

	// Create all our windows and the tray icon.  This is done after all other chances
	// to return early due to an error have passed, above.
	if (g_script->CreateWindows() != OK)
		return CRITICAL_ERROR;

	// store main thread window as first item
	g_ahkThreads[0][0] = (UINT_PTR)g_hWnd;

	// At this point, it is nearly certain that the script will be executed.

	// v1.0.48.04: Turn off buffering on stdout so that "FileAppend, Text, *" will write text immediately
	// rather than lazily. This helps debugging, IPC, and other uses, probably with relatively little
	// impact on performance given the OS's built-in caching.  I looked at the source code for setvbuf()
	// and it seems like it should execute very quickly.  Code size seems to be about 75 bytes.
	setvbuf(stdout, NULL, _IONBF, 0); // Must be done PRIOR to writing anything to stdout.

	if (g_MaxHistoryKeys && (g_KeyHistory = (KeyHistoryItem *)malloc(g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
		ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
	//else leave it NULL as it was initialized in globaldata.

#ifdef CONFIG_DEBUGGER
	// Initiate debug session now if applicable.
	if (!g_DebuggerHost.IsEmpty() && g_Debugger.Connect(g_DebuggerHost, g_DebuggerPort) == DEBUGGER_E_OK)
	{
		g_Debugger.Break();
	}
#endif

	// set exception filter to disable hook before exception occures to avoid system/mouse freeze
	g_ExceptionHandler = AddVectoredExceptionHandler(NULL,DisableHooksOnException);
	// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
	// top part (the auto-execute part) of the script so that they will be in effect even if the
	// top part is something that's very involved and requires user interaction:
	Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
	g_HSSameLineAction = false; // `#Hotstring X` should not affect Hotstring().
	g_SuspendExempt = false; // #SuspendExempt should not affect Hotkey()/Hotstring().

	g_script->mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
	Var *clipboard_var = g_script->FindOrAddVar(_T("Clipboard")); // Add it if it doesn't exist, in case the script accesses "Clipboard" via a dynamic variable.
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

unsigned __stdcall ThreadMain(LPTSTR lpScriptCmdLine)
{
	g = &g_startup;
	// Init any globals not in "struct g" that need it:
	LPTSTR lpCmdLine = _T("");
	LPTSTR lpFileName = _T("");
	size_t lenScript = _tcslen(lpScriptCmdLine);
	if (*(lpScriptCmdLine + lenScript + 1))
		lenScript += _tcslen(lpScriptCmdLine + lenScript + 1);
	if (*(lpScriptCmdLine + lenScript + 2))
		lenScript += _tcslen(lpScriptCmdLine + lenScript + 2);
	if (!(g_lpScript = tmalloc(lenScript + 3)))
	{
		_endthreadex(CRITICAL_ERROR);
		return CRITICAL_ERROR;
	}
	memcpy(g_lpScript, lpScriptCmdLine, (lenScript + 3) * sizeof(TCHAR));
	if (*(g_lpScript + _tcslen(g_lpScript) + 1))
		lpCmdLine = g_lpScript + _tcslen(g_lpScript) + 1;
	if (*(g_lpScript + _tcslen(g_lpScript) + _tcslen(lpCmdLine) + 2))
		lpFileName = g_lpScript + _tcslen(g_lpScript) + _tcslen(lpCmdLine) + 2;
	free(lpScriptCmdLine);
	g_clip = new Clipboard();
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
	for (int f = 0;;f=f ? f : 1)
	{
		g_script = new Script();
		g_script->Construct();
		g_MsgMonitor = new MsgMonitorList();

		if (!f)
		{
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

			UserMenu::sMenuPrototype = Object::CreatePrototype(_T("Menu"), Object::sPrototype, UserMenu::sMembers, _countof(UserMenu::sMembers));
			UserMenu::sMenuBarPrototype = Object::CreatePrototype(_T("MenuBar"), UserMenu::sMenuPrototype);

			GuiType::sPrototype = Object::CreatePrototype(_T("Gui"), Object::sPrototype, GuiType::sMembers, _countof(GuiType::sMembers));
			GuiControlType::sPrototype = Object::CreatePrototype(_T("Gui.Control"), Object::sPrototype, GuiControlType::sMembers, _countof(GuiControlType::sMembers));
			GuiControlType::sPrototypeList = Object::CreatePrototype(_T("Gui.List"), GuiControlType::sPrototype, GuiControlType::sMembersList, _countof(GuiControlType::sMembersList));
			InputObject::sPrototype = Object::CreatePrototype(_T("InputHook"), Object::sPrototype, InputObject::sMembers, _countof(InputObject::sMembers));
			
			Object::sClass = Object::CreateClass(_T("Object"), Object::sClassPrototype, Object::sPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
			Object::sClassClass = Object::CreateClass(_T("Class"), Object::sClass, Object::sClassPrototype, static_cast<ObjectMethod>(&Object::New<Object>));
			Array::sClass = Object::CreateClass(_T("Array"), Object::sClass, Array::sPrototype, static_cast<ObjectMethod>(&Array::New<Array>));
			Map::sClass = Object::CreateClass(_T("Map"), Object::sClass, Map::sPrototype, static_cast<ObjectMethod>(&Map::New<Map>));

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
		}
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
		bool restart_mode = false;

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

		int argc = 0;
		LPTSTR *argv = *lpCmdLine ? CommandLineToArgvW(lpCmdLine, &argc) : NULL;
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
		int i;
		for (i = 0; i < argc; ++i) // Start at 0 it does not contain the program name.
		{
			LPTSTR param = argv[i]; // For performance and convenience.
			// Insist that switches be an exact match for the allowed values to cut down on ambiguity.
			// For example, if the user runs "CompiledScript.exe /find", we want /find to be considered
			// an input parameter for the script rather than a switch:
			if (!_tcsicmp(param, _T("/ErrorStdOut")))
				g_script->mErrorStdOut = true;
			else if (!_tcsicmp(param, _T("/iLib"))) // v1.0.47: Build an include-file so that ahk2exe can include library functions called by the script.
			{
				++i; // Consume the next parameter too, because it's associated with this one.
				if (i >= argc) // Missing the expected filename parameter.
				{
					_endthreadex(CRITICAL_ERROR);
					return CRITICAL_ERROR;
				}
				// For performance and simplicity, open/create the file unconditionally and keep it open until exit.
				g_script->mIncludeLibraryFunctionsThenExit = new TextFile;
				if (!g_script->mIncludeLibraryFunctionsThenExit->Open(param, TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
				{
					_endthreadex(CRITICAL_ERROR);
					return CRITICAL_ERROR;
				}
			}
			else // since this is not a recognized switch, the end of the [Switches] section has been reached (by design).
			{
				break; // No more switches allowed after this point.
			}
		}

		if (Var *var = g_script->FindOrAddVar(_T("A_Args"), 6, VAR_DECLARE_SUPER_GLOBAL))
		{
			// Store the remaining args in an array and assign it to "A_Args".
			// If there are no args, assign an empty array so that A_Args[1]
			// and A_Args.Length() don't cause an error.
			auto args = Array::FromArgV(__targv + i, __argc - i);
			if (!args)
			{
				if (argv)
					LocalFree(argv); // free memory allocated by CommandLineToArgvW
				_endthreadex(CRITICAL_ERROR);
				return CRITICAL_ERROR;  // Realistically should never happen.
			}
			var->AssignSkipAddRef(args);
		}
		else
		{
			if (argv)
				LocalFree(argv); // free memory allocated by CommandLineToArgvW
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;
		}
		if (argv)
			LocalFree(argv); // free memory allocated by CommandLineToArgvW

		global_init(*g);  // Set defaults prior to the below, since below might override them for AutoIt2 scripts.

		// Set up the basics of the script:
		if (g_script->Init(*g, g_lpScript, 0, g_hInstance, true) != OK)  // Set up the basics of the script, using the above.
		{
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;
		}
		// Set g_default now, reflecting any changes made to "g" above, in case AutoExecSection(), below,
		// never returns, perhaps because it contains an infinite loop (intentional or not):
		CopyMemory(&g_default, g, sizeof(global_struct));

		// Use FindOrAdd vs Add for maintainability, although it shouldn't already exist:
		if (!(g_ErrorLevel = g_script->FindOrAddVar(_T("ErrorLevel"))))
		{
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR; // Error.  Above already displayed it for us.
		}
		// Initialize the var state to zero:
		g_ErrorLevel->Assign(ERRORLEVEL_NONE);

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

		LineNumberType load_result = g_script->LoadFromText(g_lpScript);
		if (load_result == LOADING_FAILED) // Error during load (was already displayed by the function call).
		{
			if (g_script->mIncludeLibraryFunctionsThenExit)
				g_script->mIncludeLibraryFunctionsThenExit->Close(); // Flush its buffer to disk.
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;  // Should return this value because PostQuitMessage() also uses it.
		}
		if (!load_result) // LoadFromFile() relies upon us to do this check.  No lines were loaded, so we're done.
		{
			_endthreadex(0);
			return 0;
		}
		if (*lpFileName)
			g_script->mFileName = lpFileName;
		// Create all our windows and the tray icon.  This is done after all other chances
		// to return early due to an error have passed, above.
		if (g_script->CreateWindows() != OK)
		{
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;
		}

		// save g_hWnd in threads global array used for exiting threads, debugger and probably more
		for (int i = 1;; i++)
		{
			if (i == MAX_AHK_THREADS)
			{
				_endthreadex(CRITICAL_ERROR);
				return CRITICAL_ERROR;
			}
			if (!IsWindow((HWND)g_ahkThreads[i][0]))
			{
				g_ahkThreads[i][0] = (UINT_PTR)g_hWnd;
				g_ahkThreads[i][1] = (UINT_PTR)g_script;
				g_ahkThreads[i][2] = (UINT_PTR)Line::sSourceFile;
				g_ahkThreads[i][3] = (UINT_PTR)Line::sSourceFileCount;
				g_ahkThreads[i][4] = (UINT_PTR)&g_startup;
				g_ahkThreads[i][5] = (UINT_PTR)g_ThreadID;
				g_ahkThreads[i][6] = (UINT_PTR)((PMYTEB)NtCurrentTeb())->ThreadLocalStoragePointer;
				break;
			}
		}
		// Set AutoHotkey.dll its main window (ahk_class AutoHotkey) bottom so it does not receive Reload or ExitApp Message send e.g. when Reload message is sent.
		//SetWindowPos(g_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOSIZE);

		// At this point, it is nearly certain that the script will be executed.

		// v1.0.48.04: Turn off buffering on stdout so that "FileAppend, Text, *" will write text immediately
		// rather than lazily. This helps debugging, IPC, and other uses, probably with relatively little
		// impact on performance given the OS's built-in caching.  I looked at the source code for setvbuf()
		// and it seems like it should execute very quickly.  Code size seems to be about 75 bytes.
		setvbuf(stdout, NULL, _IONBF, 0); // Must be done PRIOR to writing anything to stdout.

		if (g_MaxHistoryKeys && !g_KeyHistory && (g_KeyHistory = (KeyHistoryItem *)malloc(g_MaxHistoryKeys * sizeof(KeyHistoryItem))))
			ZeroMemory(g_KeyHistory, g_MaxHistoryKeys * sizeof(KeyHistoryItem)); // Must be zeroed.
		//else leave it NULL as it was initialized in globaldata.

		/* // Use ExceptionHandler only in main process
		// set exception filter to disable hook before exception occures to avoid system/mouse freeze
		// also when dll will crash, it will only exit dll thread and try to destroy it, leaving the exe process running
		// specify 1 so dll handler runs before exe handler
		g_ExceptionHandler = AddVectoredExceptionHandler(1,DisableHooksOnException);
		*/
		// set exception filter to disable hook before exception occures to avoid system/mouse freeze
		g_ExceptionHandler = AddVectoredExceptionHandler(NULL, DisableHooksOnException);

		// Activate the hotkeys, hotstrings, and any hooks that are required prior to executing the
		// top part (the auto-execute part) of the script so that they will be in effect even if the
		// top part is something that's very involved and requires user interaction:
		Hotkey::ManifestAllHotkeysHotstringsHooks(); // We want these active now in case auto-execute never returns (e.g. loop)
		g_HSSameLineAction = false; // `#Hotstring X` should not affect Hotstring().
		g_SuspendExempt = false; // #SuspendExempt should not affect Hotkey()/Hotstring().
		//Hotkey::InstallKeybdHook();
		//Hotkey::InstallMouseHook();
		//if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
		//	AddRemoveHooks(3);

		Var *clipboard_var = g_script->FindOrAddVar(_T("Clipboard")); // Add it if it doesn't exist, in case the script accesses "Clipboard" via a dynamic variable.

		g_script->mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
		
		//Sleep(20);
		// Notify that thread was created successfully
		TCHAR buf[MAX_INTEGER_LENGTH];
		sntprintf(buf, _countof(buf), _T("ahk%d"), GetCurrentThreadId());
		HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, true, buf);
		SetEvent(hEvent);
		CloseHandle(hEvent);
		// Run the auto-execute part at the top of the script (this call might never return):
		ResultType result = g_script->AutoExecSection();
		if (!result) // Can't run script at all. Due to rarity, just abort.
		{
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;
		}
		else if (g_Reloading)
		{
			g_Reloading = false;
			delete g_script;
			continue;
		}
		// REMEMBER: The call above will never return if one of the following happens:
		// 1) The AutoExec section never finishes (e.g. infinite loop).
		// 2) The AutoExec function uses the Exit or ExitApp command to terminate the script.
		// 3) The script isn't persistent and its last line is reached (in which case an ExitApp is implicit).

		// Call it in this special mode to kick off the main event loop.
		// Be sure to pass something >0 for the first param or it will
		// return (and we never want this to return):
		//MsgSleep(SLEEP_INTERVAL, WAIT_FOR_MESSAGES);
		delete g_SimpleHeap;
		_endthreadex(0);
	}
	return 0; // Never executed; avoids compiler warning.
}
#endif // _USRDLL