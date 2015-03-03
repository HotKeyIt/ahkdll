GlobalVar(name){
    mVar:=ScriptStruct().mVar
    Loop % ScriptStruct().mVarCount
        If (var:=mVar[A_Index]).mName = name
            return var
    return []
}