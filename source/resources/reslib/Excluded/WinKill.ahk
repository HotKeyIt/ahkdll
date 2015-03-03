WinKill(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
  static SMTO_ABORTIFHUNG:=2,WM_CLOSE:=16,PROCESS_ALL_ACCESS:=2035711
	; Use WM_CLOSE vs. SC_CLOSE in this case, since the target window is slightly more likely to
	; respond to that:
	if !DllCall("SendMessageTimeout","PTR",hWnd,"UInt",WM_CLOSE,"PTR", 0,"PTR", 0,"UInt", SMTO_ABORTIFHUNG,"UInt", 500,"UInt*", dwResult){ ; Wait up to 500ms.
		pid := DllCall("GetWindowThreadProcessId","PTR",hWnd,"PTR",0)
		If hProcess := pid ? DllCall("OpenProcess","UInt",PROCESS_ALL_ACCESS,"Char", FALSE,"UInt", pid) : 0	{
			DllCall("TerminateProcess","PTR",hProcess,"UInt",0)
			DllCall("CloseHandle","PTR",hProcess)
		}
	}
}