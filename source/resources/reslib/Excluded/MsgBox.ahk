MsgBox(uiType:="",aTitle:="",aText:="",aTimeout:=0,aOwner:=0){
	static MSGBOX_TEXT_SIZE:=1024 * 8,DIALOG_TITLE_SIZE:=1024,MAX_MSGBOXES:=7,QS_ALLEVENTS:=1215,MB_SETFOREGROUND:=65536,WM_COMMNOTIFY:=68,AHK_TIMEOUT:=-2,g:=GlobalStruct(),g_script:=ScriptStruct()
	; Set the below globals so that any WM_TIMER messages dispatched by this call to
	; MsgBox() (which may result in a recursive call back to us) know not to display
	; any more MsgBoxes:
	if (g_script.mMessageBoxes > MAX_MSGBOXES + 1)  ; +1 for the final warning dialog.  Verified correct.
		return

	if (g_script.mMessageBoxes = MAX_MSGBOXES){
		; Do a recursive call to self so that it will be forced to the foreground.
		; But must increment this so that the recursive call allows the last MsgBox
		; to be displayed:
		++g_script.mMessageBoxes
		MsgBox The maximum number of MsgBoxes has been reached.
		--g_script.mMessageBoxes
		return
	}

	; Set these in case the caller explicitly called it with a NULL, overriding the default:
	if (aTitle="")
		; If available, the script's filename seems a much better title in case the user has
		; more than one script running:
		aTitle := A_ScriptName ? A_ScriptName : "AutoHotkey " A_AhkVersion
  If (aText="")
    aText:=uiType=""?"Press OK to continue.":uiType,uiType:=0
	
	uiType |= MB_SETFOREGROUND  ; Always do these so that caller doesn't have to specify.

	; In the below, make the MsgBox owned by the topmost window rather than our main
	; window, in case there's another modal dialog already displayed.  The forces the
	; user to deal with the modal dialogs starting with the most recent one, which
	; is what we want.  Otherwise, if a middle dialog was dismissed, it probably
	; won't be able to return which button was pressed to its original caller.
	; UPDATE: It looks like these modal dialogs can't own other modal dialogs,
	; so disabling this:
	/*
	HWND topmost = GetTopWindow(g_hWnd);
	if (!topmost) ; It has no child windows.
		topmost = g_hWnd
	*/

	; Unhide the main window, but have it minimized.  This creates a task
	; bar button so that it's easier the user to remember that a dialog
	; is open and waiting (there are probably better ways to handle
	; this whole thing).  UPDATE: This isn't done because it seems
	; best not to have the main window be inaccessible until the
	; dialogs are dismissed (in case ever want to use it to display
	; status info, etc).  It seems that MessageBoxes get their own
	; task bar button when they're not AppModal, which is one of the
	; main things I wanted, so that's good too):
;	if (!IsWindowVisible(g_hWnd) || !IsIconic(g_hWnd))
;		ShowWindowAsync(g_hWnd, SW_SHOWMINIMIZED);

	/*
	If the script contains a line such as "#y::MsgBox, test", and a hotkey is used
	to activate Windows Explorer and another hotkey is then used to invoke a MsgBox,
	that MsgBox will be psuedo-minimized or invisible, even though it does have the
	input focus.  This attempt to fix it didn't work, so something is probably checking
	the physical key state of LWIN/RWIN and seeing that they're down:
	modLR_type modLR_now = GetModifierLRState();
	modLR_type win_keys_down = modLR_now & (MOD_LWIN | MOD_RWIN);
	if (win_keys_down)
		SetModifierLRStateSpecific(win_keys_down, modLR_now, KEYUP);
	*/

	; Note: Even though when multiple messageboxes exist, they might be
	; destroyed via a direct call to their WindowProc from our message pump's
	; DispatchMessage, or that of another MessageBox's message pump, it
	; appears that MessageBox() is designed to be called recursively like
	; this, since it always returns the proper result for the button on the
	; actual MessageBox it originally invoked.  In other words, if a bunch
	; of Messageboxes are displayed, and this user dismisses an older
	; one prior to dealing with a newer one, all the MessageBox()
	; return values will still wind up being correct anyway, at least
	; on XP.  The only downside to the way this is designed is that
	; the keyboard can't be used to navigate the buttons on older
	; messageboxes (only the most recent one).  This is probably because
	; the message pump of MessageBox() isn't designed to properly dispatch
	; keyboard messages to other MessageBox window instances.  I tried
	; to fix that by making our main message pump handle all messages
	; for all dialogs, but that turns out to be pretty complicated, so
	; I abandoned it for now.

	; Note: It appears that MessageBox windows, and perhaps all modal dialogs in general,
	; cannot own other windows.  That's too bad because it would have allowed each new
	; MsgBox window to be owned by any previously existing one, so that the user would
	; be forced to close them in order if they were APPL_MODAL.  But it's not too big
	; an issue since the only disadvantage is that the keyboard can't be use to
	; to navigate in MessageBoxes other than the most recent.  And it's actually better
	; the way it is now in the sense that the user can dismiss the messageboxes out of
	; order, which might (in rare cases) be desirable.

	if (aTimeout > 2147483) ; This is approximately the max number of seconds that SetTimer can handle.
		aTimeout := 2147483
	if (aTimeout < 0) ; But it can be equal to zero to indicate no timeout at all.
		aTimeout := 0.1  ; A value that might cue the user that something is wrong.
	; For the above:
	; MsgBox's smart comma handling will usually prevent negatives due to the fact that it considers
	; a negative to be part of the text param.  But if it does happen, timeout after a short time,
	; which may signal the user that the script passed a bad parameter.

	; v1.0.33: The following is a workaround for the fact that an MsgBox with only an OK button
	; doesn't obey EndDialog()'s parameter:
	g.DialogHWND := 0
	g.MsgBoxTimedOut := false

	; At this point, we know a dialog will be displayed.  See macro's comments for details:
	; DIALOG_PREP ; Must be done prior to POST_AHK_DIALOG() below.
  thread_was_critical := g.ThreadIsCritical
	g.ThreadIsCritical := false
	g.AllowThreadToBeInterrupted := true
	if (DllCall("GetQueueStatus","UInt",QS_ALLEVENTS)>>16)&0xFFFF ; See DIALOG_PREP for explanation.
		Sleep -1
	DllCall("PostMessage","PTR",A_ScriptHwnd,"UInt", WM_COMMNOTIFY, "PTR", 1027,"PTR", aTimeout * 1000)

	++g_script.mMessageBoxes  ; This value will also be used as the Timer ID if there's a timeout.
	g.MsgBoxResult := DllCall("MessageBox","PTR",aOwner,"Str",SubStr(aText,1,MSGBOX_TEXT_SIZE),"Str",SubStr(aTitle,1,DIALOG_TITLE_SIZE),"UInt", uType)
	--g_script.mMessageBoxes
	; Above's use of aOwner: MsgBox, FileSelect, and other dialogs seem to consider aOwner to be NULL
	; when aOwner is minimized or hidden.

	g.ThreadIsCritical := thread_was_critical,g.AllowThreadToBeInterrupted:=!thread_was_critical

	; If there's a timer, kill it for performance reasons since it's no longer needed.
	; Actually, this isn't easy to do because we don't know what the HWND of the MsgBox
	; window was, so don't bother:
	;if (aTimeout != 0.0)
	;	KillTimer(...);

;	if (!g_script.mMessageBoxes)
;		ShowWindowAsync(g_hWnd, SW_HIDE);  ; Hide the main window if it no longer has any child windows.
;	else

	; This is done so that the next message box of ours will be brought to the foreground,
	; to remind the user that they're still out there waiting, and for convenience.
	; Update: It seems bad to do this in cases where the user intentionally wants the older
	; messageboxes left in the background, to deal with them later.  So, in those cases,
	; we might be doing more harm than good because the user's foreground window would
	; be intrusively changed by this:
	;WinActivateOurTopDialog();

	; The following comment is apparently not always true -- sometimes the AHK_TIMEOUT from
	; EndDialog() is received correctly.  But I haven't discovered the circumstances of how
	; and why the behavior varies:
	; Unfortunately, it appears that MessageBox() will return zero rather
	; than AHK_TIMEOUT that was specified in EndDialog() at least under WinXP.
	if (g.MsgBoxTimedOut || (!g.MsgBoxResult && aTimeout > 0)) ; v1.0.33: Added g->MsgBoxTimedOut, see comment higher above.
		; Assume it timed out rather than failed, since failure should be VERY rare.
		g.MsgBoxResult := AHK_TIMEOUT
	; else let the caller handle the display of the error message because only it knows
	; whether to also tell the user something like "the script will not continue".
	return
}