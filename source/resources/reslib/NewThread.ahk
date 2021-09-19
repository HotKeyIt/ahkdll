NewThread(s:="Persistent",p:="",t:="AutoHotkey",w:=0xFFFFFFFF,h:=0){
  static Newthread_CloseWindows:=CallbackCreate(NewThread,,2)
  if (IsInteger(s)){
    if IsInteger(p){
      DetectHiddenWindows:=A_DetectHiddenWindows
      A_DetectHiddenWindows := 1
      If (WinGetClass("ahk_id " s)!="AutoHotkey")
        SendMessage_(s,16,0,0) ;WM_CLOSE:=16
      A_DetectHiddenWindows:=DetectHiddenWindows
      return true
    }
    success:=true
    if hThread:=OpenThread(THREAD_ALL_ACCESS:=2032639,true,s)
      EnumThreadWindows(s,Newthread_CloseWindows,0),success:=QueueUserAPC(NumGet(GetProcAddress(t,"g_ThreadExitApp"),"PTR"),hThread,s),CloseHandle(hThread),PostThreadMessage(s, 0, 0, 0)
    return success
  }
  if !IsInteger(h)
	h:=LoadLibrary(h)
  ahkFunction:=GetProcAddress(h:=h?h:A_ModuleHandle,"ahkFunction"),ahkPostFunction:=GetProcAddress(h,"ahkPostFunction")
  ,thread:={ThreadID:ThreadID:=s=""?GetCurrentThreadId():DllCall(GetProcAddress(h,"NewThread"), "Str", (w?"SetEvent(" hEvent:=CreateEvent() ")`n":"") s, "Str", p, "Str", t, "CDECL UInt")}
  ,thread._ahkFunction:=DynaCall(ahkFunction,"s==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID)
  ,thread._ahkPostFunction:=DynaCall(ahkPostFunction,"i==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID)
  ,thread.ahkFunction:=DynaCall(ahkFunction,"s==sssssssssssui","","","","","","","","","","","",ThreadID)
  ,thread.ahkPostFunction:=DynaCall(ahkPostFunction,"i==sssssssssssui","","","","","","","","","","","",ThreadID)
  ,thread.addScript:=DynaCall(GetProcAddress(h,"addScript"),"ut==siui","",0,ThreadID)
  ,thread.ahkExec:=DynaCall(GetProcAddress(h,"ahkExec"),"ut==sui","",ThreadID)
  ,thread.ahkExecuteLine:=DynaCall(GetProcAddress(h,"ahkExecuteLine"),"ut==utuiuiui",0,0,0,ThreadID)
  ,thread.ahkFindFunc:=DynaCall(GetProcAddress(h,"ahkFindFunc"),"ut==sui","",ThreadID)
  ,thread.ahkFindLabel:=DynaCall(GetProcAddress(h,"ahkFindLabel"),"ut==sui","",ThreadID)
  ,thread.ahkgetvar:=DynaCall(GetProcAddress(h,"ahkgetvar"),"s==suiui","",0,ThreadID)
  ,thread.ahkassign:=DynaCall(GetProcAddress(h,"ahkassign"),"i==ssui","","",ThreadID)
  ,thread.ahkLabel:=DynaCall(GetProcAddress(h,"ahkLabel"),"ui==suiui","",0,ThreadID)
  ,thread.ahkPause:=DynaCall(GetProcAddress(h,"ahkPause"),"ui==sui","",ThreadID)
  ,thread.ahkReady:=DynaCall(GetProcAddress(h,"ahkReady"),"ui==ui",ThreadID)
  ,thread.ahkterminate:=NewThread.Bind(ThreadID,,h)
  if w
    WaitForSingleObject(hEvent,w),CloseHandle(hEvent)
  return thread
}