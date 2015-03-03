WinMoveTop(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
  static HWND_TOP:=0,SWP_NOMOVE:=2,SWP_NOSIZE:=1,SWP_NOACTIVATE:=16
	if target_window := aTitle aText=""?WinExist():WinExist(aTitle,aText,aExcludeTitle,aExcludeText){
		; Note: SWP_NOACTIVATE must be specified otherwise the target window often fails to move:
		if !DllCall("SetWindowPos,","PTR",target_window,"PTR", HWND_TOP,"Int", 0,"Int", 0,"Int", 0,"Int", 0,"UInt", SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE){
			ErrorLevel := TRUE
			return
		}
	}
	ErrorLevel := FALSE
}