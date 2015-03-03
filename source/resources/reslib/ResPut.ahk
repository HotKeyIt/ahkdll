ResPut(ByRef data,size,dll,name,type:=10,language:=0){
	return !(hUpdate:=BeginUpdateResourceW(dll))?0:(result:=UpdateResource(hUpdate,type,name,language,IsByRef(data)?&data:data,size),result:=EndUpdateResource(hUpdate,!result),result)
}