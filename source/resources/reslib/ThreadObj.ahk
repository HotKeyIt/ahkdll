ThreadObj(script,cmdLine:="",title:=""){
	ThreadID:=NewThread("A_ParentThread:=ObjShare(" ObjShare(new _ThreadClass) ")`nNumPut(ObjShare(ThreadObj_Class())," getvar(pObjShare:=0) ",`"PTR`")`nSetEvent(" hEvent:=CreateEvent() ")`n" script,cmdLine,title)
	,WaitForSingleObject(hEvent,1000),CloseHandle(hEvent)
	return pObjShare?ObjShare(pObjShare):""
}
ThreadObj_Class(){
	return new _ThreadClass
}
ThreadObj_IsRunning(thread){
	ComError:=ComObjError(false)
	v:=thread.VarContent("A_ScriptHwnd")
	ComObjError(ComError)
	return v!=""
}
Class _ThreadClass {
	__Get(k){
		global
		return %k%
	}
	__Set(k,v){
		global
		return %k%:=v
	}
	PostCall(func,p*){
		static postcall,params
		postcall:=func,params:=p
		SetTimer,PostCall,-1
		return
		PostCall:
			MsgBox % %postcall%(params*)
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
	GoTo(label){
		if !IsLabel(label)
			return 0
		SetTimer % label,-1
		Return 1
	}
	GoSub(label){
		if !IsLabel(label)
			return 0
		GoSub % label
		return 1
	}
	ExitApp(){
		SetTimer,ExitApp,-1
		Return
		ExitApp:
		ExitApp
	}
}