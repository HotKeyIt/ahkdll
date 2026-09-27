ResPutFile(File,dll,name,type:=10,language:=0){
If hUpdate:=DllCall("BeginUpdateResource","Str",dll,"Int",0,"PTR"){
FileGetSize,nSize,%File%
If nSize {
FileRead,bin,*c %File%
result:=DllCall("UpdateResource","PTR",hUpdate,type+0=""?"Str":"PTR",type,name+0=""?"Str":"PTR",name,"UShort",language,"PTR",&Bin,"UInt",nSize)
}
result:=EndUpdateResource(hUpdate,!result)
} else return 0
return result
}