DynaRun(s,pn:="",pr:="",exe:=""){
static AhkPath:="`"" A_AhkPath "`"" (A_IsCompiled||(A_IsDll&&DllCall(A_AhkPath "\ahkgetvar","Str","A_IsCompiled","Uint",0,"Uint",0,"CDecl"))?" /E":"")
,h2o:="B29C2D1CA2C24A57BC5E208EA09E162F(){`nPLACEHOLDERB29C2D1CA2C24A57BC5E208EA09E162Fh:='',dmp:=Buffer(sz:=StrLen(hex)//2,0)`nLoop Parse, hex`nIf (`"`"!=h.=A_LoopField) && !Mod(A_Index,2)`nNumPut(`"UChar`",`"0x`" h,dmp,A_Index/2-1),h:=`"`"`nreturn ObjLoad(dmp.Ptr)`n}`n"
local dmp:="",_s:="",p1:="",p2:="",Arg:=""
if (-1=p1:=DllCall("CreateNamedPipe","str",pf:="\\.\pipe\" (pn!=""?pn:"AHK" A_TickCount),"uint",2,"uint",0,"uint",255,"uint",0,"uint",0,"Ptr",0,"Ptr",0))
|| (-1=p2:=DllCall("CreateNamedPipe","str",pf,"uint",2,"uint",0,"uint",255,"uint",0,"uint",0,"Ptr",0,"Ptr",0))
Return 0
try
Run (exe?exe:AhkPath) " `"" pf "`" " (IsObject(pr)?"": " " pr),,"UseErrorLevel HIDE",&P
catch
return (DllCall("CloseHandle","Ptr",p1),DllCall("CloseHandle","Ptr",p2),0)
If IsObject(pr) {
dmp:=ObjDump(pr),hex:=BinToHex(dmp.Ptr,dmp.Size)
While _hex:=SubStr(Hex,1 + (A_Index-1)*16370,16370)
_s.= "hex" (A_Index=1?":":".") "=`"" _hex "`"`n"
Arg:=StrReplace(h2o,"PLACEHOLDERB29C2D1CA2C24A57BC5E208EA09E162F",_s) "global A_Args:=B29C2D1CA2C24A57BC5E208EA09E162F()`n"
}
DllCall("ConnectNamedPipe","Ptr",p1,"Ptr",0),DllCall("CloseHandle","Ptr",p1),DllCall("ConnectNamedPipe","Ptr",p2,"Ptr",0)
if !DllCall("WriteFile","Ptr",p2,"Wstr",s:=chr(0xfeff) Arg s,"UInt",StrLen(s)*2+4,"uint*",0,"Ptr",0)
P:=0
DllCall("CloseHandle","Ptr",p2)
Return P
}