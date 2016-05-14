AhkExported(){
    static lib
	if !lib
		lib:=GetModuleHandle(),ahkFunction:=GetProcAddress(lib,"ahkFunction"),ahkPostFunction:=GetProcAddress(lib,"ahkPostFunction"),addFile:=GetProcAddress(lib,"addFile"),addScript:=GetProcAddress(lib,"addScript")
			,ahkExec:=GetProcAddress(lib,"ahkExec"),ahkExecuteLine:=GetProcAddress(lib,"ahkExecuteLine"),ahkFindFunc:=GetProcAddress(lib,"ahkFindFunc"),ahkFindLabel:=GetProcAddress(lib,"ahkFindLabel")
			,ahkgetvar:=GetProcAddress(lib,"ahkgetvar"),ahkassign:=GetProcAddress(lib,"ahkassign"),ahkLabel:=GetProcAddress(lib,"ahkLabel"),ahkPause:=GetProcAddress(lib,"ahkPause")
			,ahkIsUnicode:=GetProcAddress(lib,"ahkIsUnicode"),ahkReady:=GetProcAddress(lib,"ahkReady"),threadID:=NumGet(GetProcAddress(lib,"g_MainThreadID"),"UInt")
	thread:={(""):ThreadID
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
	return thread
}