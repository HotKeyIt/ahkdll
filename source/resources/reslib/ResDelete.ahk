ResDelete(dll,name,type,language:=0){
	return !(hUpdate:=BeginUpdateResourceW(dll))?0:(result:=EndUpdateResource(hUpdate,!result:=UpdateResource(hUpdate,name,type,language)),result)
}