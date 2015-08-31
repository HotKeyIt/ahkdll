ResPutFile(File,dll,name,type:=10,language:=0){
If hUpdate:=BeginUpdateResourceW(dll){
If nSize:=FileGetSize(File)
bin:=FileRead("*c " File),result:=UpdateResource(hUpdate,name,type,language,&Bin,nSize)
result:=EndUpdateResource(hUpdate,!result)
} else return 0
return result
}