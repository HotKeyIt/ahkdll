SendMessage(msg,ByRef wparam:=0,ByRef lparam:=0,control:="",aTitle:="",aText:="",aExTitle:="",aExText:="",aTimeout:=5000){
	static SMTO_ABORTIFHUNG:=2
		if (   !(target_window = WinExist(aTitle, aText, aExTitle, aExText))
		|| !(control_window = control!=""?ControlGet("Hwnd",control,"ahk_id " target_window):target_window)   ){ ; Relies on short-circuit boolean order.
		 ; aUseSend: Need a special value to distinguish this from numeric reply-values.
     ErrorLevel:="ERROR"
		return
  }
	; v1.0.40.05: Support the passing of a literal (quoted) string by checking whether the
	; original/raw arg's first character is '"'.  The avoids the need to put the string into a
	; variable and then pass something like &MyVar.
	
	; Fixed for v1.0.48.04: Make copies of the wParam and lParam variables (if eligible for updating) prior
	; to sending the message in case the message triggers a callback or OnMessage function, which would be
	; likely to change the contents of the mArg array before we're doing using them after the Post/SendMsg.
	; Seems best to do the above EVEN for PostMessage in case it can ever trigger a SendMessage internally
	; (I seem to remember that the OS sometimes converts a PostMessage call into a SendMessage if the
	; origin and destination are the same thread.)
	; v1.0.43.06: If either wParam or lParam contained the address of a variable, update the mLength
	; member after sending the message in case the receiver of the message wrote something to the buffer.
	; This is similar to the way "Str" parameters work in DllCall.
	if !DllCall("SendMessageTimeout","PTR",control_window,"UInt", msg,"PTR",IsByRef(wParam)&&wParam+0!=""?wparam:&wParam,"PTR",IsByRef(lParam)&&lParam+0!=""?lparam:&lParam,"UInt", SMTO_ABORTIFHUNG,"UInt", timeout,"UInt*", dwResult){
		ErrorLevel:="ERROR"
		return  ; Need a special value to distinguish this from numeric reply-values.
	}
	ErrorLevel:=dwResult

  If IsByRef(wParam)
    VarSetCapacity(wParam,-1)
  If IsByRef(lParam)
    VarSetCapacity(lParam,-1)
	return
}