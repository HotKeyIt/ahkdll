ResGetEx(ByRef data,dll,name,type:=10,language:=0,freeLibrary:=1){
static find:=DynaCall("FindResourceEx","t=tssh"),lib:=DynaCall("LoadLibrary","t=s"),free:=DynaCall("FreeLibrary","t"),move:=DynaCall("RtlMoveMemory","ttt"),load:=DynaCall("LoadResource","t=tt"),lock:=DynaCall("LockResource","t=t"),size:=DynaCall("SizeofResource","ui=tt")
pdata:=lock[hResData:=load[hModule,hResource:=find[hModule:=lib[dll],name,type,language]]]
,VarsetCapacity(data,sz:=size[hModule,hResource]),move[&data,pData,sz]
if (freeLibrary && hModule!=DllCall("GetModuleHandle","PTR",0))
free[hModule]
return sz
}