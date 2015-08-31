FileReplace(ByRef data,file,Encoding:=""){
	If FileExist(file)&&!FileDelete(file)
		Return ErrorLevel:=1,0
	if Encoding
		return FileAppend(data,file)
	else return FileAppend(data,file,Encoding)
}