ObjShare(obj){
	static IDispatch:=Buffer(16), init := NumPut("int64",0x46000000000000c0, NumPut("int64", 0x20400, IDispatch))
	if IsObject(obj)
		return LresultFromObject(IDispatch.Ptr, 0, ObjPtr(obj))
	else if ObjectFromLresult(obj, IDispatch.Ptr, 0, getvar(com:=0))
		return MsgBox(A_ThisFunc ": LResult Object could not be created")
	return ComValue(9,com,1)
}