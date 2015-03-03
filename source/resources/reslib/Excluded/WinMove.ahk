WinMove(aTitle:="",aText:="",aX:="",aY:="",aWidth:="",aHeight:="",aExcludeTitle:="",aExcludeText:=""){
  static rect:=Struct("left,top,right,bottom")
	if !DllCall("GetWindowRect","PTR",aTitle aText=""?WinExist():WinExist(aTitle, aText, aExcludeTitle, aExcludeText),"PTR",rect[]){
    ErrorLevel:=1
		return
  }
	ErrorLevel:=!DllCall("MoveWindow","PTR",target_window,"Int",aX!=""?aX:rect.left,"Int",aY!=""?aY:rect.top,"Int",aWidth!=""?aWidth:rect.right-rect.left,"Int",aHeight!=""aHeight:rect.bottom - rect.top,"Char",TRUE)  ; Do repaint.
	Sleep % A_WinDelay
	return
}