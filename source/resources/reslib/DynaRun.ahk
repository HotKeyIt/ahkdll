DynaRun(s,pn:="",pr:="",exe:=""){
static AhkPath
if !AhkPath
AhkPath:="`"" A_AhkPath "`"" (A_IsCompiled||(A_IsDll&&DllCall(A_AhkPath "\ahkgetvar","Str","A_IsCompiled","CDecl"))?" /E":"")
if (-1=p1:=DllCall("CreateNamedPipe","str",pf:="\\.\pipe\" (pn!=""?pn:"AHK" A_TickCount),"uint",2,"uint",0,"uint",255,"uint",0,"uint",0,"Ptr",0,"Ptr",0))
|| (-1=p2:=DllCall("CreateNamedPipe","str",pf,"uint",2,"uint",0,"uint",255,"uint",0,"uint",0,"Ptr",0,"Ptr",0))
Return 0
Run % (exe?exe:AhkPath) " `"" pf "`" " pr,,UseErrorLevel HIDE,P
If ErrorLevel
return DllCall("CloseHandle","Ptr",p1),DllCall("CloseHandle","Ptr",p2),0
DllCall("ConnectNamedPipe","Ptr",p1,"Ptr",0),DllCall("CloseHandle","Ptr",p1),DllCall("ConnectNamedPipe","Ptr",p2,"Ptr",0)
if !DllCall("WriteFile","Ptr",p2,"Wstr",s:=chr(0xfeff) s,"UInt",StrLen(s)*2+4,"uint*",0,"Ptr",0)
P:=0
DllCall("CloseHandle","Ptr",p2)
Return P
}