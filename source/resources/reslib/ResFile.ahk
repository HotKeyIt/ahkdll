ResFile(File,dll,name,type,language:=0){
static begin:=DynaCall("BeginUpdateResourceW","t=wi"),update:=DynaCall("UpdateResource","tssuhtui"),end:=DynaCall("EndUpdateResource","ti")
If hUpdate:=begin[dll]{
FileRead,Bin,*c %File%
FileGetSize,nSize,%File%
If nSize
result:=update[hUpdate,name,type,language,&Bin,nSize]
result:=end[hUpdate,!result]
} else result:=0
return result
}