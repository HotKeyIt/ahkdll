FileReplace(data,file,Options:=""){
	If FileExist(file)
		FileDelete(file)
	if !Options
		return FileAppend(data,file)
	else return FileAppend(data,file,Options)
}