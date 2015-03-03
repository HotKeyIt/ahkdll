Resource(){
static r:={find:DynaCall("FindResourceW","t=ttt"),lib:DynaCall("LoadLibraryW","t=w"),free:DynaCall("FreeLibrary","t")
,move:DynaCall("RtlMoveMemory","ttt"),load:DynaCall("LoadResource","t=tt"),lock:DynaCall("LockResource","t=t")
,size:DynaCall("SizeofResource","ui=tt"),begin:DynaCall("BeginUpdateResourceW","t=wi")
,update:DynaCall("UpdateResource","tttuhtui"),end:DynaCall("EndUpdateResource","ti")}
return r
}
ResourceReadData(ByRef data,dll,section,key,freeLibrary:=1){
static r:=Resource()
pdata:=r.lock[hResData:=r.load[hModule,hResource:=r.find[hModule:=r.lib[dll],section+0=""?&section:section,key+0=""?&key:key]]]
,VarsetCapacity(data,SizeofResource:=r.size[hModule,hResource])
,r.move[&data,pData,SizeofResource]
if freeLibrary
r.free[hModule]
return SizeofResource
}
ResourceCreateDll(F:="empty.dll") {
static CreateFile:=DynaCall("CreateFile","ui=tuitttuiuiuiuitui","",0x40000000,2,0,2),write:=DynaCall("WriteFile","ttuiui*t")
,CloseHandle:=DynaCall("CloseHandle","t"),Trg,init:=VarSetCapacity(Trg,1536,0)+Numput(TS,Trg,192),TS:=DateAdd(A_NowUTC,1970,"s")
,Src:="0X5A4DY3CXB8YB8X4550YBCX2014CYCCX210E00E0YD0X7010BYD8X400YE4X1000YE8X1000YECX78AE0000YF0X1000YF4X200YF8X10005YFCX10005Y100X4Y108X3000Y10CX200Y114X2Y118X40000Y11CX2000Y120X100000Y124X1000Y12CX10Y140X1000Y144X10Y158X2000Y15CX8Y1B0X7273722EY1B4X63Y1B8X10Y1BCX1000Y1C0X200Y1C4X200Y1D4X40000040Y1D8X6C65722EY1DCX636FY1E0X8Y1E4X2000Y1E8X200Y1ECX400Y1FCX42000040"
Loop,Parse,Src,XY
Mod(A_Index,2)?O:="0x" A_LoopField:NumPut("0x" A_LoopField,Trg,O)
If 0<hF:=CreateFile[&F]
Write[&hF,&Trg,1536,w],CloseHandle[hF]
Return w=1536
}
ResourceExist(dll,section,key,freeLibrary:=1){
static r:=Resource()
return Exist:=r.find[hModule:=r.lib[dll],section+0=""?&section:section,key+0=""?&key:key]?1:0,r.free[hModule],Exist
}
ResourceUpdate(ByRef data,size,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin[dll]{
result:=r.update[hUpdate,section+0=""?&section:section,key+0=""?&key:key,language,&data,size]
result:=r.end[hUpdate,!result]
} else result:=0
return result
}
ResourceDelete(dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin[dll]{
result:=r.update[hUpdate,section+0=""?&section:section,key+0=""?&key:key,language,0,0]
result:=r.end[hUpdate,!result]
} else result:=0
return result
}
ResourceUpdateFile(File,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin[dll]{
FileRead,Bin,*c %File%
FileGetSize,nSize,%File%
If nSize
result:=r.update[hUpdate,section+0=""?&section:section,key+0=""?&key:key,language,&Bin,nSize]
result:=r.end[hUpdate,!result]
} else result:=0
return result
}
ResourceUpdateFiles(Folder,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin[dll]{
_result:=0
Loop, Folder "\*"
{
FileRead,Bin,*c %A_LoopFileLongPath%
FileGetSize,nSize,%A_LoopFileLongPath%
If nSize
If result:=r.update[hUpdate,section+0=""?&section:section,key+0=""?&key:key,language,&Bin,nSize]
_result++
}
r.end[hUpdate,!_result]
} else _result:=0
return _result
}