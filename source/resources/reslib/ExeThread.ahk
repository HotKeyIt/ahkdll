exethread(s:="",p:="",t:="",w:=0){
  static lib
  if !lib
    lib:=GetModuleHandle(),ahkFunction:=GetProcAddress(lib,"ahkFunction"),ahkPostFunction:=GetProcAddress(lib,"ahkPostFunction"),addFile:=GetProcAddress(lib,"addFile"),addScript:=GetProcAddress(lib,"addScript"),ahkExec:=GetProcAddress(lib,"ahkExec"),ahkExecuteLine:=GetProcAddress(lib,"ahkExecuteLine"),ahkFindFunc:=GetProcAddress(lib,"ahkFindFunc"),ahkFindLabel:=GetProcAddress(lib,"ahkFindLabel"),ahkgetvar:=GetProcAddress(lib,"ahkgetvar"),ahkassign:=GetProcAddress(lib,"ahkassign"),ahkLabel:=GetProcAddress(lib,"ahkLabel"),ahkPause:=GetProcAddress(lib,"ahkPause"),ahkReady:=GetProcAddress(lib,"ahkReady")
	thread:={ThreadID:ThreadID:=s=""?GetCurrentThreadId():NewThread((w?"SetEvent(" hEvent:=CreateEvent() ")`n":"") s,p,t)}
    thread.DefineMethod("_ahkFunction",DynaCall(ahkFunction,"s==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID))
	thread.DefineMethod("_ahkPostFunction",DynaCall(ahkPostFunction,"i==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID))
    thread.DefineMethod("ahkFunction",DynaCall(ahkFunction,"s==sssssssssssui","","","","","","","","","","","",ThreadID))
    thread.DefineMethod("ahkPostFunction",DynaCall(ahkPostFunction,"i==sssssssssssui","","","","","","","","","","","",ThreadID))
    thread.DefineMethod("addFile",DynaCall(addFile,"ut==siui","",0,ThreadID))
    thread.DefineMethod("addScript",DynaCall(addScript,"ut==siui","",0,ThreadID))
    thread.DefineMethod("ahkExec",DynaCall(ahkExec,"ut==sui","",ThreadID))
    thread.DefineMethod("ahkExecuteLine",DynaCall(ahkExecuteLine,"ut==utuiuiui",0,0,0,ThreadID))
    thread.DefineMethod("ahkFindFunc",DynaCall(ahkFindFunc,"ut==sui","",ThreadID))
    thread.DefineMethod("ahkFindLabel",DynaCall(ahkFindLabel,"ut==sui","",ThreadID))
    thread.DefineMethod("ahkgetvar",DynaCall(ahkgetvar,"s==suiui","",0,ThreadID))
    thread.DefineMethod("ahkassign",DynaCall(ahkassign,"i==ssui","","",ThreadID))
    thread.DefineMethod("ahkLabel",DynaCall(ahkLabel,"ui==suiui","",0,ThreadID))
    thread.DefineMethod("ahkPause",DynaCall(ahkPause,"ui==sui","",ThreadID))
    thread.DefineMethod("ahkReady",DynaCall(ahkReady,"ui==ui",ThreadID))
	thread.ahkterminate:=Func("Exethread_exit").Bind(ThreadID)
	if w
		WaitForSingleObject(hEvent,w),CloseHandle(hEvent)
	return thread
}
Exethread_exit(ThreadID){
	static pExitApp:=NumGet(GetProcAddress(0,"g_ThreadExitApp"),"PTR"),Exethread_CloseWindows:=CallbackCreate("Exethread_CloseWindows")
	if hThread:=OpenThread(THREAD_ALL_ACCESS:=2032639,true,ThreadID)
		success:=QueueUserAPC(pExitApp,hThread,0),CloseHandle(hThread),PostThreadMessage(ThreadID, 0, 0, 0),EnumThreadWindows(ThreadID,Exethread_CloseWindows,0)
	return success
}
Exethread_CloseWindows(hwnd,value){
	If (WinGetClass("ahk_id " hwnd)!="AutoHotkey")
		SendMessage_(hwnd,16,0,0) ;WM_CLOSE:=16
	return true
}