WinClose(aTitle:="",aText:="",wait:=0,aExcludeTitle:="",aExcludeText:=""){
  static WM_CLOSE:=16
  start:=A_TickCount
  If DllCall("PostMessage","PTR",aWin:=aTitle aText=""?WinExist():WinExist(aTitle,aText,aExcludeTitle,aExcludeText),"UInt",WM_CLOSE,"PTR",0,"PTR",0){
    While DllCall("IsWindow","PTR",aWin) && wait>A_TickCount-start
      Sleep 10
    ErrorLevel:=DllCall("IsWindow","PTR",aWin)
  } else ErrorLevel:=1
}