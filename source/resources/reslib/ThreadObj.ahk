Class ThreadObj {
  static Call(script,cmdLine:="",title:=""){
    if IsObject(script){
      v:=0
      try v:=script.A_ScriptHwnd
      return v!=0
    }
    ThreadID:=DllCall(A_AhkPath "\NewThread", "Str", "A_ParentThread:=ObjShare(" ObjShare(super()) ")`nNumPut(`"PTR`",ObjShare(ThreadObj.New())," getvar(pObjShare:=0) ")`nSetEvent(" hEvent:=CreateEvent() ")`n" script, "Str", cmdLine, "Str", title, "CDecl UInt")
    ,WaitForSingleObject(hEvent,1000),CloseHandle(hEvent)
    return pObjShare?ObjShare(pObjShare):""
  }
  static New(){
    return super()
  }
	__Get(k,p){
		global
		return %k%
	}
	__Set(k,p,v){
		global
		return %k%:=v
	}
	PostCall(func,p*){
		static postcall,params
		postcall:=func,params:=p
		SetTimer "PostCall",-1
		return
		PostCall:
			%postcall%(params*)
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
	ExitApp(){
		SetTimer ExitApp,-1
		Return
		ExitApp:
		ExitApp
	}
}