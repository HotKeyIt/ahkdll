WinGetPidList(WinTitle:="",WinText:="",ExcludeTitle:="",ExcludeText:=""){
  static p:=Buffer(40960),i:=0,EnumProcesses:=DynaCall(DllCall("GetProcAddress","PTR",DllCall("LoadLibrary","STR","psapi","PTR"),"AStr","EnumProcesses","PTR"),"tuiui",p.Ptr,40960,getvar(i))
  local dhw,dht,ps:=[]
  EnumProcesses()
  ,IsObject(WinTitle) ? (dhw:=A_DetectHiddenWindows,DetectHiddenWindows(WinTitle[2]),WinTitle:=WinTitle[1]) : ""
  ,IsObject(WinText) ? (dht:=A_DetectHiddenText,DetectHiddenText(WinText[2]),WinText:=WinText[1]) : ""
  Loop i/4
    If WinExist(WinTitle " ahk_pid " pid:=NumGet(p,(A_Index-1)*4,"UInt"),WinText,ExcludeTitle,ExcludeText)
      ps.Push(pid)
  IsSet(dhw) ? DetectHiddenWindows(dhw) : "",IsSet(dht) ? DetectHiddenText(dht) : ""
  return ps
}