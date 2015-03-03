StaticVar(name,func){
    if FindFunc(func)&&mVar:=(f:=Struct("ScriptStruct(Func)",FindFunc(func))).mStaticVar
        Loop % f.mStaticVarCount
            If (var:=mVar[A_Index]).mName = name
                return var
    return []
}