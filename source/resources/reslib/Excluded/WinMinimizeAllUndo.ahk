WinMinimizeAllUndo(){
  static WM_COMMAND:=273
  DllCall("PostMessage","PTR",WinExist("ahk_class Shell_TrayWnd"),"UInt",WM_COMMAND,"PTR",416,"PTR",0)
  Sleep % A_WinDelay
}