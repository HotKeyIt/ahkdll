ResDelete(dll,name,type:=10,language:=1033){
	return !(hUpdate:=DllCall("BeginUpdateResource","Str",dll,"Int",0,"PTR"))?0:(result:=EndUpdateResource(hUpdate,!result:=DllCall("UpdateResource","PTR",hUpdate,type+0=""?"Str":"PTR",type,name+0=""?"Str":"PTR",name,"UShort",language,"PTR",0,"UInt",0)),result)
}