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

#ifndef globaldata_h
#define globaldata_h

#include "hook.h" // For KeyHistoryItem and probably other things.
#include "clipboard.h"  // For the global clipboard object
#include "script.h" // For the global script object
#include "os_version.h" // For the global OS_Version object
#include "MemoryModule.h"
#include "Debugger.h"

thread_local extern FuncLibrary sLib[FUNC_LIB_COUNT]; // function libraries
extern LPSTR g_hWinAPI, g_hWinAPIlowercase; // loads WinAPI functions definitions from resource
thread_local extern SimpleHeap *g_SimpleHeap;
extern HRSRC g_hResource;		// for compiled AutoHotkey.exe
extern HMEMORYMODULE g_hNTDLL;
extern HMEMORYMODULE g_hKERNEL32;
extern HMEMORYMODULE g_hCRYPT32;
extern _QueryPerformanceCounter g_QPC;
extern _CryptStringToBinaryA g_CS2BA;
extern _CryptStringToBinaryW g_CS2BW;
extern double g_QPCtimer;
extern double g_QPCfreq;
extern LPTSTR g_result_to_return_dll[4095]; //HotKeyIt H2 for ahkgetvar return value.	
extern FuncAndToken g_FuncAndTokenToReturn[4095];    // for ahkFunction
thread_local extern int g_ExitCode;
thread_local extern bool g_Reloading;
//#else
EXPORT FARPROC g_ThreadExitApp;
extern UINT_PTR g_ahkThreads[MAX_AHK_THREADS][7];
thread_local extern PVOID g_original_tls;
thread_local extern CRITICAL_SECTION g_CriticalTLSCallback;
thread_local extern HMODULE g_hMemoryModule;
thread_local extern DWORD g_MainThreadID;
extern HINSTANCE g_hInstance;
EXPORT extern DWORD g_FirstThreadID;
thread_local extern DWORD g_HookThreadID;
thread_local extern LPTSTR g_lpScript;
thread_local extern bool g_UseStdLib;
thread_local extern int g_MapCaseSense;
thread_local extern SymbolType g_DefaultObjectValueType;
thread_local extern LPTSTR g_DefaultObjectValue;
thread_local extern SymbolType g_DefaultArrayValueType;
thread_local extern LPTSTR g_DefaultArrayValue;
thread_local extern SymbolType g_DefaultMapValueType;
thread_local extern LPTSTR g_DefaultMapValue;
thread_local extern ATOM g_ClassRegistered;
thread_local extern CRITICAL_SECTION g_CriticalRegExCache;
thread_local extern BuiltInFunc* sIsSetFunc;
//#ifdef _USRDLL
//thread_local extern CRITICAL_SECTION g_CriticalHeapBlocks;
//#endif
thread_local extern DWORD g_CriticalObjectTimeOut;
thread_local extern DWORD g_CriticalObjectSleepTime;
thread_local extern LPWSTR g_WindowClassMain;
thread_local extern LPWSTR g_WindowClassGUI;

thread_local extern UINT g_DefaultScriptCodepage;

thread_local extern bool g_ReturnNotExit;	// for ahkExec/addScript
thread_local extern bool g_DestroyWindowCalled;
thread_local extern HWND g_hWnd;  // The main window
thread_local extern HWND g_hWndEdit;  // The edit window, child of main.
thread_local extern HFONT g_hFontEdit;
thread_local extern HACCEL g_hAccelTable; // Accelerator table for main menu shortcut keys.

typedef int (WINAPI *StrCmpLogicalW_type)(LPCWSTR, LPCWSTR);
thread_local extern StrCmpLogicalW_type g_StrCmpLogicalW;
thread_local extern WNDPROC g_TabClassProc;

thread_local extern modLR_type g_modifiersLR_logical;   // Tracked by hook (if hook is active).
thread_local extern modLR_type g_modifiersLR_logical_non_ignored;
thread_local extern modLR_type g_modifiersLR_physical;  // Same as above except it's which modifiers are PHYSICALLY down.
thread_local extern modLR_type g_modifiersLR_numpad_mask;  // Shift keys temporarily released by Numpad.
thread_local extern modLR_type g_modifiersLR_ctrlaltdel_mask; // For excluding AltGr from Ctrl+Alt+Del handling.

thread_local extern key_type *pPrefixKey;

#ifdef FUTURE_USE_MOUSE_BUTTONS_LOGICAL
thread_local extern WORD g_mouse_buttons_logical; // A bitwise combination of MK_LBUTTON, etc.
#endif

#define STATE_DOWN 0x80
#define STATE_ON 0x01
thread_local extern BYTE g_PhysicalKeyState[VK_ARRAY_COUNT];
thread_local extern bool g_BlockWinKeys;
thread_local extern DWORD g_AltGrExtraInfo;

thread_local extern BYTE g_MenuMaskKeyVK; // For #MenuMaskKey.
thread_local extern USHORT g_MenuMaskKeySC;

// If a SendKeys() operation takes longer than this, hotkey's modifiers won't be pressed back down:
thread_local extern int g_HotkeyModifierTimeout;
thread_local extern int g_ClipboardTimeout;

thread_local extern HHOOK g_KeybdHook;
thread_local extern HHOOK g_MouseHook;
thread_local extern HHOOK g_PlaybackHook;
thread_local extern bool g_ForceLaunch;
thread_local extern bool g_WinActivateForce;
thread_local extern bool g_RunStdIn;
thread_local extern WarnMode g_Warn_LocalSameAsGlobal;
thread_local extern WarnMode g_Warn_Unreachable;
thread_local extern WarnMode g_Warn_VarUnset;
thread_local extern SingleInstanceType g_AllowOnlyOneInstance;
thread_local extern bool g_TargetWindowError;
thread_local extern bool g_persistent;
thread_local extern bool g_NoTrayIcon;
thread_local extern bool g_AllowMainWindow;
thread_local extern bool g_DeferMessagesForUnderlyingPump;
thread_local extern bool g_MainTimerExists;
thread_local extern bool g_InputTimerExists;
thread_local extern bool g_DerefTimerExists;
thread_local extern bool g_SoundWasPlayed;
thread_local extern bool g_IsSuspended;
thread_local extern bool g_OnExitIsRunning;
thread_local extern BOOL g_AllowInterruption;
thread_local extern int g_nLayersNeedingTimer;
thread_local extern int g_nThreads;
thread_local extern int g_nPausedThreads;
thread_local extern int g_MaxHistoryKeys;
thread_local extern DWORD g_InputTimeoutAt;

thread_local extern UCHAR g_MaxThreadsPerHotkey;
thread_local extern int g_MaxThreadsTotal;
thread_local extern UINT g_MaxHotkeysPerInterval;
thread_local extern UINT g_HotkeyThrottleInterval;
thread_local extern bool g_MaxThreadsBuffer;
thread_local extern bool g_SuspendExempt;
thread_local extern bool g_SuspendExemptHS;
thread_local extern SendLevelType g_InputLevel;
thread_local extern HotkeyCriterion *g_FirstHotCriterion, *g_LastHotCriterion;

// Global variables for #HotIf (expression). See globaldata.cpp for comments.
thread_local extern UINT g_HotExprTimeout;
thread_local extern HWND g_HotExprLFW;
thread_local extern HotkeyCriterion *g_FirstHotExpr, *g_LastHotExpr;

thread_local extern int g_ScreenDPI;
thread_local extern MenuTypeType g_MenuIsVisible;
thread_local extern int g_nMessageBoxes;
thread_local extern int g_nFileDialogs;
thread_local extern int g_nFolderDialogs;
thread_local extern GuiType *g_firstGui, *g_lastGui;
thread_local extern HWND g_hWndToolTip[MAX_TOOLTIPS];
thread_local extern MsgMonitorList *g_MsgMonitor;

thread_local extern UCHAR g_SortCaseSensitive;
thread_local extern bool g_SortNumeric;
thread_local extern bool g_SortReverse;
thread_local extern int g_SortColumnOffset;
thread_local extern IObject *g_SortFunc;
thread_local extern ResultType g_SortFuncResult;

#define g_DerefChar   '%' // As of v2 these are constant, so even more parts of the code assume they
#define g_EscapeChar  '`' // are at their usual default values to reduce code size/complexity.
#define g_delimiter   ',' // Also, g_delimiter was never used in expressions (i.e. for SYM_COMMA).
#define g_CommentChar ';'

// Hot-string vars:
thread_local extern TCHAR g_HSBuf[HS_BUF_SIZE];
thread_local extern int g_HSBufLength;
thread_local extern HWND g_HShwnd;

// Hot-string global settings:
thread_local extern int g_HSPriority;
thread_local extern int g_HSKeyDelay;
thread_local extern SendModes g_HSSendMode;
thread_local extern SendRawType g_HSSendRaw;
thread_local extern bool g_HSCaseSensitive;
thread_local extern bool g_HSConformToCase;
thread_local extern bool g_HSDoBackspace;
thread_local extern bool g_HSOmitEndChar;
thread_local extern bool g_HSEndCharRequired;
thread_local extern bool g_HSDetectWhenInsideWord;
thread_local extern bool g_HSDoReset;
thread_local extern bool g_HSResetUponMouseClick;
thread_local extern bool g_HSSameLineAction;
thread_local extern TCHAR g_EndChars[HS_MAX_END_CHARS + 1];

// Global objects:
thread_local extern input_type *g_input;
EXTERN_SCRIPT;
EXTERN_CLIPBOARD;
EXTERN_OSVER;

thread_local extern HICON g_IconSmall;
thread_local extern HICON g_IconLarge;
thread_local extern DWORD g_OriginalTimeout;
EXTERN_G;
thread_local extern global_struct g_startup, *g_array;
#define g_default g_array[0]

thread_local extern CString g_WorkingDir;
thread_local extern LPTSTR g_WorkingDirOrig;

thread_local extern Debugger *g_Debugger;
// jackieku: modified to hold the buffer.
thread_local extern CStringA g_DebuggerHost;
thread_local extern CStringA g_DebuggerPort;

thread_local extern bool g_ForceKeybdHook;
thread_local extern ToggleValueType g_ForceNumLock;
thread_local extern ToggleValueType g_ForceCapsLock;
thread_local extern ToggleValueType g_ForceScrollLock;

thread_local extern ToggleValueType g_BlockInputMode;
thread_local extern bool g_BlockInput;  // Whether input blocking is currently enabled.
thread_local extern bool g_BlockMouseMove; // Whether physical mouse movement is currently blocked via the mouse hook.

extern Action g_act[];
extern int g_ActionCount;
extern Action g_old_act[];
extern int g_OldActionCount;

extern key_to_vk_type g_key_to_vk[];
extern key_to_sc_type g_key_to_sc[];
extern int g_key_to_vk_count;
extern int g_key_to_sc_count;

thread_local extern KeyHistoryItem *g_KeyHistory;
thread_local extern int g_KeyHistoryNext;
thread_local extern DWORD g_HistoryTickNow;
thread_local extern DWORD g_HistoryTickPrev;
thread_local extern HWND g_HistoryHwndPrev;
thread_local extern DWORD g_TimeLastInputPhysical;
thread_local extern DWORD g_TimeLastInputKeyboard;
thread_local extern DWORD g_TimeLastInputMouse;

extern ULONGLONG g_crypt_code[6];
extern TCHAR *g_default_pwd[32];
// 9 might be better than 10 because if the granularity/timer is a little
// off on certain systems, a Sleep(10) might really result in a Sleep(20),
// whereas a Sleep(9) is almost certainly a Sleep(10) on OS's such as
// NT/2k/XP.  UPDATE: Roundoff issues with scripts having
// even multiples of 10 in them, such as "Sleep,300", shouldn't be hurt
// by this because they use GetTickCount() to verify how long the
// sleep duration actually was.  UPDATE again: Decided to go back to 10
// because I'm pretty confident that that always sleeps 10 on NT/2k/XP
// unless the system is under load, in which case any Sleep between 0
// and 20 inclusive seems to sleep for exactly(?) one timeslice.
// A timeslice appears to be 20ms in duration.  Anyway, using 10
// allows "SetKeyDelay, 10" to be really 10 rather than getting
// rounded up to 20 due to doing first a Sleep(10) and then a Sleep(1).
// For now, I'm avoiding using timeBeginPeriod to improve the resolution
// of Sleep() because of possible incompatibilities on some systems,
// and also because it may degrade overall system performance.
// UPDATE: Will get rounded up to 10 anyway by SetTimer().  However,
// future OSs might support timer intervals of less than 10.
#define SLEEP_INTERVAL 10
#define SLEEP_INTERVAL_HALF (int)(SLEEP_INTERVAL / 2)

enum OurTimers {TIMER_ID_MAIN = MAX_MSGBOXES + 2 // The first timers in the series are used by the MessageBoxes.  Start at +2 to give an extra margin of safety.
	, TIMER_ID_UNINTERRUPTIBLE // Obsolete but kept as a a placeholder for backward compatibility, so that this and the other the timer-ID's stay the same, and so that obsolete IDs aren't reused for new things (in case anyone is interfacing these OnMessage() or with external applications).
	, TIMER_ID_AUTOEXEC, TIMER_ID_INPUT, TIMER_ID_DEREF, TIMER_ID_REFRESH_INTERRUPTIBILITY};

// MUST MAKE main timer and uninterruptible timers associated with our main window so that
// MainWindowProc() will be able to process them when it is called by the DispatchMessage()
// of a non-standard message pump such as MessageBox().  In other words, don't let the fact
// that the script is displaying a dialog interfere with the timely receipt and processing
// of the WM_TIMER messages, including those "hidden messages" which cause DefWindowProc()
// (I think) to call the TimerProc() of timers that use that method.
// Realistically, SetTimer() called this way should never fail?  But the event loop can't
// function properly without it, at least when there are suspended subroutines.
// MSDN docs for SetTimer(): "Windows 2000/XP: If uElapse is less than 10,
// the timeout is set to 10." TO GET CONSISTENT RESULTS across all operating systems,
// it may be necessary never to pass an uElapse parameter outside the range USER_TIMER_MINIMUM
// (0xA) to USER_TIMER_MAXIMUM (0x7FFFFFFF).
#define SET_MAIN_TIMER \
if (!g_MainTimerExists)\
	g_MainTimerExists = SetTimer(g_hWnd, TIMER_ID_MAIN, SLEEP_INTERVAL, (TIMERPROC)NULL);
// v1.0.39 for above: Apparently, one of the few times SetTimer fails is after the thread has done
// PostQuitMessage. That particular failure was causing an unwanted recursive call to ExitApp(),
// which is why the above no longer calls ExitApp on failure.  Here's the sequence:
// Someone called ExitApp (such as the max-hotkeys-per-interval warning dialog).
// ExitApp() removes the hooks.
// The hook-removal function calls MsgSleep() while waiting for the hook-thread to finish.
// MsgSleep attempts to set the main timer so that it can judge how long to wait.
// The timer fails and calls ExitApp even though a previous call to ExitApp is currently underway.

// See AutoExecSectionTimeout() for why g->AllowThreadToBeInterrupted is used rather than the other var.
// The below also sets g->ThreadStartTime and g->UninterruptibleDuration.  Notes about this:
// In case the AutoExecute section takes a long time (or never completes), allow interruptions
// such as hotkeys and timed subroutines after a short time. Use g->AllowThreadToBeInterrupted
// vs. g_AllowInterruption in case commands in the AutoExecute section need exclusive use of
// g_AllowInterruption (i.e. they might change its value to false and then back to true,
// which would interfere with our use of that var).
// From MSDN: "When you specify a TimerProc callback function, the default window procedure calls the
// callback function when it processes WM_TIMER. Therefore, you need to dispatch messages in the calling thread,
// even when you use TimerProc instead of processing WM_TIMER."  My: This is why all TimerProc type timers
// should probably have an associated window rather than passing NULL as first param of SetTimer().
//
// UPDATE v1.0.48: g->ThreadStartTime and g->UninterruptibleDuration were added so that IsInterruptible()
// won't make the AutoExec section interruptible prematurely.  In prior versions, KILL_AUTOEXEC_TIMER() did this,
// but with the new IsInterruptible() function, doing it in KILL_AUTOEXEC_TIMER() wouldn't be reliable because
// it might already have been done by IsInterruptible() [or vice versa], which might provide a window of
// opportunity in which any use of Critical by the AutoExec section would be undone by the second timeout.
// More info: Since AutoExecSection() never calls InitNewThread(), it never used to set the uninterruptible
// timer.  Instead, it had its own timer.  But now that IsInterruptible() checks for the timeout of
// "Thread Interrupt", AutoExec might become interruptible prematurely unless it uses the new method below.
//
// v2.0: There's no longer an AutoExec timer, so this just sets the uninterruptible duration.
#define SET_AUTOEXEC_TIMER(aTimeoutValue) \
{\
	g->AllowThreadToBeInterrupted = false;\
	g->ThreadStartTime = GetTickCount();\
	g->UninterruptibleDuration = aTimeoutValue;\
}

#define SET_INPUT_TIMER(aTimeoutValue, aTimeoutAt) \
{ \
g_InputTimeoutAt = aTimeoutAt; \
g_InputTimerExists = SetTimer(g_hWnd, TIMER_ID_INPUT, aTimeoutValue, InputTimeout); \
}


// For this one, SetTimer() is called unconditionally because our caller wants the timer reset
// (as though it were killed and recreated) unconditionally.  MSDN's comments are a little vague
// about this, but testing shows that calling SetTimer() against an existing timer does completely
// reset it as though it were killed and recreated.  Note also that g_hWnd is used vs. NULL so that
// the timer will fire even when a msg pump other than our own is running, such as that of a MsgBox.
#define SET_DEREF_TIMER(aTimeoutValue) g_DerefTimerExists = SetTimer(g_hWnd, TIMER_ID_DEREF, aTimeoutValue, DerefTimeout);
#define LARGE_DEREF_BUF_SIZE (4*1024*1024)

#define KILL_MAIN_TIMER \
if (g_MainTimerExists && KillTimer(g_hWnd, TIMER_ID_MAIN))\
	g_MainTimerExists = false;

#define KILL_INPUT_TIMER \
if (g_InputTimerExists && KillTimer(g_hWnd, TIMER_ID_INPUT))\
	g_InputTimerExists = false;

#define KILL_DEREF_TIMER \
if (g_DerefTimerExists && KillTimer(g_hWnd, TIMER_ID_DEREF))\
	g_DerefTimerExists = false;

static inline void AddGuiToList(GuiType* gui)
{
	gui->mNextGui = NULL;
	gui->mPrevGui = g_lastGui;
	if (g_lastGui)
		g_lastGui->mNextGui = gui;
	g_lastGui = gui;
	if (!g_firstGui)
		g_firstGui = gui;
	// AddRef() is not called here because we want the GUI to be destroyed automatically
	// when the script releases its last reference if it's not visible, or when the GUI
	// is closed if the script has no references.  See VisibilityChanged().
}

static inline void RemoveGuiFromList(GuiType* gui)
{
	if (!gui->mPrevGui && gui != g_firstGui)
		// !mPrevGui indicates this is either the first Gui or not in the list.
		// Since both conditions were met, this Gui must have been partially constructed
		// but not added to the list, and is being destroyed due to an error in Create.
		return;
	GuiType *prev = gui->mPrevGui, *&prevNext = prev ? prev->mNextGui : g_firstGui;
	GuiType *next = gui->mNextGui, *&nextPrev = next ? next->mPrevGui : g_lastGui;
	prevNext = next;
	nextPrev = prev;
}

#endif
