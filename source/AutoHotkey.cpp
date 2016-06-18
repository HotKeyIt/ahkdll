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
#include "MemoryModule.h"
#include <process.h>
// #include <vld.h> // find memory leaks

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
	PBOOLEAN BeingDebugged;
	TCHAR buf[MAX_PATH];
	FILE *fp;
	size_t size;
	HANDLE DebugPort = NULL;
	g_TlsDoExecute = true;
	HMEMORYMODULE module;
	unsigned char* data;
	// Execute only if A_IsCompiled
#ifdef _DEBUG
	module = LoadLibrary(_T("kernel32.dll"));
	g_VirtualAlloc = (_VirtualAlloc)GetProcAddress((HMODULE)module, "VirtualAlloc");
	g_VirtualFree = (_VirtualFree)GetProcAddress((HMODULE)module, "VirtualFree");
	module = LoadLibrary(_T("shlwapi.dll"));
	g_MultiByteToWideChar = (_MultiByteToWideChar)GetProcAddress((HMODULE)module, "MultiByteToWideChar");
	g_HashData = (_HashData)GetProcAddress((HMODULE)module, "HashData");
	module = LoadLibrary(_T("Crypt32.dll"));
	g_CryptStringToBinaryA = (_CryptStringToBinaryA)GetProcAddress((HMODULE)module, "CryptStringToBinaryA");
	return;
#endif
#ifndef _DEBUG
	if (!FindResource(NULL, _T("E4847ED08866458F8DD35F94B37001C0"), MAKEINTRESOURCE(RT_RCDATA)))
	{
		module = LoadLibrary(_T("kernel32.dll"));
		g_VirtualAlloc = (_VirtualAlloc)GetProcAddress((HMODULE)module, "VirtualAlloc");
		g_VirtualFree = (_VirtualFree)GetProcAddress((HMODULE)module, "VirtualFree");
		g_MultiByteToWideChar = (_MultiByteToWideChar)GetProcAddress((HMODULE)module, "MultiByteToWideChar");
		module = LoadLibrary(_T("shlwapi.dll"));
		g_HashData = (_HashData)GetProcAddress((HMODULE)module, "HashData");
		module = LoadLibrary(_T("Crypt32.dll"));
		g_CryptStringToBinaryA = (_CryptStringToBinaryA)GetProcAddress((HMODULE)module, "CryptStringToBinaryA");
		return;
	}

#ifdef _M_IX86 // compiles for x86
	BeingDebugged = (PBOOLEAN)__readfsdword(0x30) + 2;
#elif _M_AMD64 // compiles for x64
	BeingDebugged = (PBOOLEAN)__readgsqword(0x60) + 2; //0x60 because offset is doubled in 64bit
#endif
	if (*BeingDebugged) // Read the PEB
		TerminateProcess(NtCurrentProcess(), 0);
	module = (HMEMORYMODULE)LoadLibrary(_T("ntdll.dll"));
	GetModuleFileName((HMODULE)module, buf, MAX_PATH);
	FreeLibrary((HMODULE)module);
	
	fp = _tfopen(buf, _T("rb"));
	if (fp == NULL)
		return;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	data = (unsigned char*)malloc(size);
	fseek(fp, 0, SEEK_SET);
	fread(data, 1, size, fp);
	fclose(fp);
	module = MemoryLoadLibrary(data, size);
	MyNtQueryInformationProcess _NtQueryInformationProcess = (MyNtQueryInformationProcess)MemoryGetProcAddress(module, "NtQueryInformationProcess");
	if (!_NtQueryInformationProcess(NtCurrentProcess(), 7, &DebugPort, sizeof(HANDLE), NULL) && DebugPort)
	{
		MemoryFreeLibrary(module);
		free(data);
		TerminateProcess(NtCurrentProcess(), 0);
	}
	MyNtSetInformationThread _NtSetInformationThread = (MyNtSetInformationThread)MemoryGetProcAddress(module, "NtSetInformationThread");
	_NtSetInformationThread(GetCurrentThread(), 0x11, 0, 0);
	MemoryFreeLibrary(module);
	free(data);
#endif
	module = (HMEMORYMODULE)LoadLibrary(_T("kernel32.dll"));
	GetModuleFileName((HMODULE)module, buf, MAX_PATH);
	FreeLibrary((HMODULE)module);
	if (fp = _tfopen(buf, _T("rb")))
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		data = (unsigned char*)malloc(size);
		fseek(fp, 0, SEEK_SET);
		fread(data, 1, size, fp);
		fclose(fp);
		module = MemoryLoadLibrary(data, size);
		g_LoadResource = (_LoadResource)MemoryGetProcAddress(module, "LoadResource");
		g_SizeofResource = (_SizeofResource)MemoryGetProcAddress(module, "SizeofResource");
		g_LockResource = (_LockResource)MemoryGetProcAddress(module, "LockResource");
		g_VirtualAlloc = (_VirtualAlloc)MemoryGetProcAddress(module, "VirtualAlloc");
		g_VirtualFree = (_VirtualFree)MemoryGetProcAddress(module, "VirtualFree");
		g_MultiByteToWideChar = (_MultiByteToWideChar)MemoryGetProcAddress(module, "MultiByteToWideChar");
		free(data);
	}
	module = (HMEMORYMODULE)LoadLibrary(_T("Shlwapi.dll"));
	GetModuleFileName((HMODULE)module, buf, MAX_PATH);
	FreeLibrary((HMODULE)module);
	if (fp = _tfopen(buf, _T("rb")))
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		data = (unsigned char*)malloc(size);
		fseek(fp, 0, SEEK_SET);
		fread(data, 1, size, fp);
		fclose(fp);
		module = MemoryLoadLibrary(data, size);
		g_HashData = (_HashData)MemoryGetProcAddress(module, "HashData");
		free(data);
	}
	module = (HMEMORYMODULE)LoadLibrary(_T("Crypt32.dll"));
	GetModuleFileName((HMODULE)module, buf, MAX_PATH);
	FreeLibrary((HMODULE)module);
	if (fp = _tfopen(buf, _T("rb")))
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		data = (unsigned char*)malloc(size);
		fseek(fp, 0, SEEK_SET);
		fread(data, 1, size, fp);
		fclose(fp);
		module = MemoryLoadLibrary(data, size);
#ifdef _UNICODE
		g_CryptStringToBinary = (_CryptStringToBinary)MemoryGetProcAddress(module, "CryptStringToBinaryW");
#else
		g_CryptStringToBinary = (_CryptStringToBinary)MemoryGetProcAddress(module, "CryptStringToBinaryA");
#endif
		g_CryptStringToBinaryA = (_CryptStringToBinaryA)MemoryGetProcAddress(module, "CryptStringToBinaryA");
		free(data);
	}
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

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// Init any globals not in "struct g" that need it:
#ifdef _DEBUG
	g_hResource = FindResource(NULL, _T("AHK"), MAKEINTRESOURCE(RT_RCDATA));
#else
	if (g_LoadResource)
		g_hResource = FindResource(NULL, _T("E4847ED08866458F8DD35F94B37001C0"), MAKEINTRESOURCE(RT_RCDATA));
#endif
	g_hInstance = hInstance;
	g_HistoryTickPrev = GetTickCount();
	g_TimeLastInputPhysical = GetTickCount();
	g_script = new Script();
	g_clip = new Clipboard();
	g_MsgMonitor = new MsgMonitorList();
	g_MetaObject = new MetaObject();
	g_SimpleHeap = new SimpleHeap();
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
	InitializeCriticalSection(&g_CriticalAhkFunction); // used to call a function in multithreading environment.

	InitializeCriticalSection(&g_CriticalDebugger);
	g_ahkThreads[0][1] = (UINT_PTR)g_script;
	g_ahkThreads[0][4] = (UINT_PTR)&g_startup;
	g_ahkThreads[0][5] = (UINT_PTR)g_ThreadID;
	g_ahkThreads[0][6] = (UINT_PTR)((PMYTEB)NtCurrentTeb())->ThreadLocalStoragePointer;
	// v1.1.22+: This is done unconditionally, on startup, so that any attempts to read a drive
	// that has no media (and possibly other errors) won't cause the system to display an error
	// dialog that the script can't suppress.  This is known to affect floppy drives and some
	// but not all CD/DVD drives.  MSDN says: "Best practice is that all applications call the
	// process-wide SetErrorMode function with a parameter of SEM_FAILCRITICALERRORS at startup."
	// Note that in previous versions, this was done by the Drive/DriveGet commands and not
	// reverted afterward, so it affected all subsequent commands.
	SetErrorMode(SEM_FAILCRITICALERRORS);

	if (!GetCurrentDirectory(_countof(g_WorkingDir), g_WorkingDir)) // Needed for the FileSelect() workaround.
		*g_WorkingDir = '\0';
	// Unlike the below, the above must not be Malloc'd because the contents can later change to something
	// as large as MAX_PATH by means of the SetWorkingDir command.
	g_WorkingDirOrig = g_SimpleHeap->Malloc(g_WorkingDir); // Needed by the Reload command.

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
		Object *args = Object::CreateFromArgV(__targv + i, __argc - i);
		if (!args)
			return CRITICAL_ERROR;  // Realistically should never happen.
		var->AssignSkipAddRef(args);
	}
	else
		return CRITICAL_ERROR;

	global_init(*g);  // Set defaults.

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

	if (!g_TlsDoExecute)
		return 0;

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

	// MSDN: "Windows XP: If a manifest is used, InitCommonControlsEx is not required."
	// Therefore, in case it's a high overhead call, it's not done on XP or later:
	if (!g_os.IsWinXPorLater())
	{
		// Since InitCommonControls() is apparently incapable of initializing DateTime and MonthCal
		// controls, InitCommonControlsEx() must be called.
#if defined(CONFIG_WIN9X) || defined(CONFIG_WINNT4)
		// Since Ex() requires comctl32.dll 4.70+, must get the function's address dynamically
		// in case the program is running on Windows 95/NT without the updated DLL (otherwise
		// the program would not launch at all).
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
#else
		// Currently only needed on Win2k, since Win9x is unsupported.
		INITCOMMONCONTROLSEX icce;
		icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icce.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES; // ICC_WIN95_CLASSES is equivalent to calling InitCommonControls().
		InitCommonControlsEx(&icce);
#endif
	}

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
	g_lpScript = tmalloc(lenScript + 3);
	memcpy(g_lpScript, lpScriptCmdLine, (lenScript + 3) * sizeof(TCHAR));
	if (*(g_lpScript + _tcslen(g_lpScript) + 1))
		lpCmdLine = g_lpScript + _tcslen(g_lpScript) + 1;
	if (*(g_lpScript + _tcslen(g_lpScript) + _tcslen(lpCmdLine) + 2))
		lpFileName = g_lpScript + _tcslen(g_lpScript) + _tcslen(lpCmdLine) + 2;
	free(lpScriptCmdLine);
	g_clip = new Clipboard();
	InitializeCriticalSection(&g_CriticalRegExCache); // v1.0.45.04: Must be done early so that it's unconditional, so that DeleteCriticalSection() in the script destructor can also be unconditional (deleting when never initialized can crash, at least on Win 9x).
	for (;;)
	{
		g_script = new Script();
		g_MsgMonitor = new MsgMonitorList();
		g_MetaObject = new MetaObject();
		g_SimpleHeap = new SimpleHeap();

		// v1.1.22+: This is done unconditionally, on startup, so that any attempts to read a drive
		// that has no media (and possibly other errors) won't cause the system to display an error
		// dialog that the script can't suppress.  This is known to affect floppy drives and some
		// but not all CD/DVD drives.  MSDN says: "Best practice is that all applications call the
		// process-wide SetErrorMode function with a parameter of SEM_FAILCRITICALERRORS at startup."
		// Note that in previous versions, this was done by the Drive/DriveGet commands and not
		// reverted afterward, so it affected all subsequent commands.
		SetErrorMode(SEM_FAILCRITICALERRORS);

		if (!GetCurrentDirectory(_countof(g_WorkingDir), g_WorkingDir)) // Needed for the FileSelect() workaround.
			*g_WorkingDir = '\0';
		// Unlike the below, the above must not be Malloc'd because the contents can later change to something
		// as large as MAX_PATH by means of the SetWorkingDir command.
		g_WorkingDirOrig = g_SimpleHeap->Malloc(g_WorkingDir); // Needed by the Reload command.

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
		for (i = 0; i < argc; ++i) // Start at 1 because 0 contains the program name.
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
					return CRITICAL_ERROR;
				// For performance and simplicity, open/create the file unconditionally and keep it open until exit.
				g_script->mIncludeLibraryFunctionsThenExit = new TextFile;
				if (!g_script->mIncludeLibraryFunctionsThenExit->Open(argv[i], TextStream::WRITE | TextStream::EOL_CRLF | TextStream::BOM_UTF8, CP_UTF8)) // Can't open the temp file.
					return CRITICAL_ERROR;
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
			Object *args = Object::CreateFromArgV((LPTSTR*)(argv + i), argc - i);
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
		for (int i = 1; i < MAX_AHK_THREADS; i++)
		{
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
			if (i == MAX_AHK_THREADS)
			{
				_endthreadex(CRITICAL_ERROR);
				return CRITICAL_ERROR;
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
		// MSDN: "Windows XP: If a manifest is used, InitCommonControlsEx is not required."
		// Therefore, in case it's a high overhead call, it's not done on XP or later:
		if (!g_os.IsWinXPorLater())
		{
			// Since InitCommonControls() is apparently incapable of initializing DateTime and MonthCal
			// controls, InitCommonControlsEx() must be called.  But since Ex() requires comctl32.dll
			// 4.70+, must get the function's address dynamically in case the program is running on
			// Windows 95/NT without the updated DLL (otherwise the program would not launch at all).
			typedef BOOL(WINAPI *MyInitCommonControlsExType)(LPINITCOMMONCONTROLSEX);
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
		//Hotkey::InstallKeybdHook();
		//Hotkey::InstallMouseHook();
		//if (Hotkey::sHotkeyCount > 0 || Hotstring::sHotstringCount > 0)
		//	AddRemoveHooks(3);

		Var *clipboard_var = g_script->FindOrAddVar(_T("Clipboard")); // Add it if it doesn't exist, in case the script accesses "Clipboard" via a dynamic variable.

		g_script->mIsReadyToExecute = true; // This is done only after the above to support error reporting in Hotkey.cpp.
		Sleep(20);

		// Run the auto-execute part at the top of the script (this call might never return):
		ResultType result = g_script->AutoExecSection();
		if (!result) // Can't run script at all. Due to rarity, just abort.
		{
			_endthreadex(CRITICAL_ERROR);
			return CRITICAL_ERROR;
		}
		else if (result == EXIT_RELOAD)
		{
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
		MsgSleep(SLEEP_INTERVAL, WAIT_FOR_MESSAGES);
		_endthreadex(0);
	}
	return 0; // Never executed; avoids compiler warning.
}
#endif // _USRDLL