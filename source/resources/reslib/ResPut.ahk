ResPut(ByRef data,size,dll,name,type:=10,language:=1033){
	return !(hUpdate:=DllCall("BeginUpdateResource","Str",dll,"Int",0,"PTR"))?0:(result:=DllCall("UpdateResource","PTR",hUpdate,type+0=""?"Str":"PTR",type,name+0=""?"Str":"PTR",name,"UShort",language,"PTR",IsByRef(data)?&data:data,"UInt",size),result:=EndUpdateResource(hUpdate,!result),result)
}