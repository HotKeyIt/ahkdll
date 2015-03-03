ResFiles(Folder,dll,name,type,language:=0){
static begin:=DynaCall("BeginUpdateResourceW","t=wi"),update:=DynaCall("UpdateResource","tssuhtui"),end:=DynaCall("EndUpdateResource","ti")
If hUpdate:=begin[dll]{
_result:=0
LoopFiles,% Folder "\*",F
{
FileRead,Bin,*c %A_LoopFileFullPath%
FileGetSize,nSize,%A_LoopFileFullPath%
If nSize
If result:=update[hUpdate,name,type,language,&Bin,nSize]
_result++
}
end[hUpdate,!_result]
} else _result:=0
return _result
}