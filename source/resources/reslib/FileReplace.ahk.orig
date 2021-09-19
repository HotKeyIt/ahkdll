FileReplace(ByRef data,file,Options:=""){
	If FileExist(file)&&!FileDelete(file)
		Return throw Exception("Could not delete file: " file,-1)
	if !Options
		return FileAppend(data,file)
	else return FileAppend(data,file,Options)
}