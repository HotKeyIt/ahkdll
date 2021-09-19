ThreadObj(script,cmdLine:="",title:=""){
	ThreadID:=NewThread("A_ParentThread:=ObjShare(" ObjShare(ThreadObj_Class()) ")`nNumPut(`"PTR`",ObjShare(ThreadObj_Class())," getvar(pObjShare:=0) ")`nSetEvent(" hEvent:=CreateEvent() ")`n" script,cmdLine,title)
	,WaitForSingleObject(hEvent,1000),CloseHandle(hEvent)
	return pObjShare?ObjShare(pObjShare):""
}
ThreadObj_IsRunning(thread){
	v:=""
	try v:=thread.A_ScriptHwnd
	return v!=""
}
Class _ThreadClass {
	__Get(k,p){
		global
        MsgBox 'value is ' k
		return %k%
	}
	__Set(k,p,v){
		global
        MsgBox 'set value is ' k "=" v
		return %k%:=v
	}
	PostCall(func,p*){
		static postcall,params
		postcall:=func,params:=p
		SetTimer "PostCall",-1
		return
		PostCall:
			MsgBox %postcall%(params*)
		return
	}
	Call(func,p*){
		return %func%(p*)
	}
	FuncPtr(func){
		return FindFunc(func)
	}
	LabelPtr(label){
		return FindLabel(label)
	}
	Exec(code){
		ahkExec(code)
	}
	AddScript(script, execute:=0){
		return addScript(script, execute)
	}
	AddFile(file, execute := 0){
		return addFile(file, execute)
	}
	ExitApp(){
		SetTimer "ExitApp",-1
		Return
		ExitApp:
		ExitApp
	}
}
ThreadObj_Class(){
	return _ThreadClass.new()
}