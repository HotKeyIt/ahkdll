AhkExported(){
    static lib
	if !lib
		lib:=GetModuleHandle(),ahkFunction:=GetProcAddress(lib,"ahkFunction"),ahkPostFunction:=GetProcAddress(lib,"ahkPostFunction"),addFile:=GetProcAddress(lib,"addFile"),addScript:=GetProcAddress(lib,"addScript")
			,ahkExec:=GetProcAddress(lib,"ahkExec"),ahkExecuteLine:=GetProcAddress(lib,"ahkExecuteLine"),ahkFindFunc:=GetProcAddress(lib,"ahkFindFunc"),ahkFindLabel:=GetProcAddress(lib,"ahkFindLabel")
			,ahkgetvar:=GetProcAddress(lib,"ahkgetvar"),ahkassign:=GetProcAddress(lib,"ahkassign"),ahkLabel:=GetProcAddress(lib,"ahkLabel"),ahkPause:=GetProcAddress(lib,"ahkPause")
			,ahkIsUnicode:=GetProcAddress(lib,"ahkIsUnicode"),ahkReady:=GetProcAddress(lib,"ahkReady"),threadID:=NumGet(GetProcAddress(lib,"g_MainThreadID"),"UInt")
	thread:={ThreadID:ThreadID}
	,thread.DefineMethod("_ahkFunction",DynaCall(ahkFunction,"s==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID))
	,thread.DefineMethod("_ahkPostFunction",DynaCall(ahkPostFunction,"i==sttttttttttui","",0,0,0,0,0,0,0,0,0,0,ThreadID))
	,thread.DefineMethod("ahkFunction",DynaCall(ahkFunction,"s==sssssssssssui","","","","","","","","","","","",ThreadID))
	,thread.DefineMethod("ahkPostFunction",DynaCall(ahkPostFunction,"i==sssssssssssui","","","","","","","","","","","",ThreadID))
	,thread.DefineMethod("addFile",DynaCall(addFile,"ut==siui","",0,ThreadID))
	,thread.DefineMethod("addScript",DynaCall(addScript,"ut==siui","",0,ThreadID))
	,thread.DefineMethod("ahkExec",DynaCall(ahkExec,"ut==sui","",ThreadID))
	,thread.DefineMethod("ahkExecuteLine",DynaCall(ahkExecuteLine,"ut==utuiuiui",0,0,0,ThreadID))
	,thread.DefineMethod("ahkFindFunc",DynaCall(ahkFindFunc,"ut==sui","",ThreadID))
	,thread.DefineMethod("ahkFindLabel",DynaCall(ahkFindLabel,"ut==sui","",ThreadID))
	,thread.DefineMethod("ahkgetvar",DynaCall(ahkgetvar,"s==suiui","",0,ThreadID))
	,thread.DefineMethod("ahkassign",DynaCall(ahkassign,"i==ssui","","",ThreadID))
	,thread.DefineMethod("ahkLabel",DynaCall(ahkLabel,"ui==suiui","",0,ThreadID))
	,thread.DefineMethod("ahkPause",DynaCall(ahkPause,"ui==sui","",ThreadID))
	,thread.DefineMethod("ahkReady",DynaCall(ahkReady,"ui==ui",ThreadID))
	return thread
}