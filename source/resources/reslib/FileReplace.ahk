FileReplace(ByRef data,file,Options:=""){
	If FileExist(file)&&!FileDelete(file)
		Return (ErrorLevel:=1,0)
	if !Options
		return FileAppend(data,file)
	else return FileAppend(data,file,Options)
}