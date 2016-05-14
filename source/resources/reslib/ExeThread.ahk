exethread(s:="",p:="",t:=""){
  static lib
  if !lib
    lib:=GetModuleHandle(),ahkFunction:=GetProcAddress(lib,"ahkFunction"),ahkPostFunction:=GetProcAddress(lib,"ahkPostFunction"),addFile:=GetProcAddress(lib,"addFile"),addScript:=GetProcAddress(lib,"addScript")
        ,ahkExec:=GetProcAddress(lib,"ahkExec"),ahkExecuteLine:=GetProcAddress(lib,"ahkExecuteLine"),ahkFindFunc:=GetProcAddress(lib,"ahkFindFunc"),ahkFindLabel:=GetProcAddress(lib,"ahkFindLabel")
        ,ahkgetvar:=GetProcAddress(lib,"ahkgetvar"),ahkassign:=GetProcAddress(lib,"ahkassign"),ahkLabel:=GetProcAddress(lib,"ahkLabel"),ahkPause:=GetProcAddress(lib,"ahkPause")
		,ahkIsUnicode:=GetProcAddress(lib,"ahkIsUnicode"),ahkReady:=GetProcAddress(lib,"ahkReady")
	thread:={(""):ThreadID:=s=""?p:NewThread(s,p,t)
	  ,_ahkFunction:DynaCall(ahkFunction,"s==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID)
	  ,_ahkPostFunction:DynaCall(ahkPostFunction,"i==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID)
	  ,ahkFunction:DynaCall(ahkFunction,"s==sssssssssssui","","","","","","","","","","","",ThreadID)
	  ,ahkPostFunction:DynaCall(ahkPostFunction,"i==sssssssssssui","","","","","","","","","","","",ThreadID)
	  ,addFile:DynaCall(addFile,"ut==siui","",0,ThreadID)
	  ,addScript:DynaCall(addScript,"ut==siui","",0,ThreadID)
	  ,ahkExec:DynaCall(ahkExec,"ut==sui","",ThreadID)
	  ,ahkExecuteLine:DynaCall(ahkExecuteLine,"ut==utuiuiui",0,0,0,ThreadID)
	  ,ahkFindFunc:DynaCall(ahkFindFunc,"ut==sui","",ThreadID)
	  ,ahkFindLabel:DynaCall(ahkFindLabel,"ut==sui","",ThreadID)
	  ,ahkgetvar:DynaCall(ahkgetvar,"s==suiui","",0,ThreadID)
	  ,ahkassign:DynaCall(ahkassign,"i==ssui","","",ThreadID)
	  ,ahkLabel:DynaCall(ahkLabel,"ui==suiui","",0,ThreadID)
	  ,ahkPause:DynaCall(ahkPause,"ui==sui","",ThreadID)
	  ,ahkIsUnicode:DynaCall(ahkIsUnicode,"ui==")
	  ,ahkReady:DynaCall(ahkReady,"ui==ui",ThreadID)}
	thread.ahkterminate:=Func("Exethread_exit").Bind(ThreadID)
	return thread
}
Exethread_exit(ThreadID){
	static pExitApp:=NumGet(GetProcAddress(0,"g_ThreadExitApp"),"PTR"),Exethread_CloseWindows:=RegisterCallback("Exethread_CloseWindows")
	if hThread:=OpenThread(THREAD_ALL_ACCESS:=2032639,true,ThreadID)
		success:=QueueUserAPC(pExitApp,hThread,0),CloseHandle(hThread),PostThreadMessage(ThreadID, 0, 0, 0),EnumThreadWindows(ThreadID,Exethread_CloseWindows,0)
	return success
}
Exethread_CloseWindows(hwnd,value){
	If (WinGetClass("ahk_id " hwnd)!="AutoHotkey")
		SendMessage_(hwnd,16,0,0) ;WM_CLOSE:=16
	return true
}