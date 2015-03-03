DriveGetList(d:="ALL"){
static s:={CDROM:5,REMOVABLE:2,FIXED:3,REMOTE:4,RAMDISK:6,UNKNOWN:0,ALL:256}
DllCall("SetErrorMode","Int",1)
Loop % 26	{
if ((s[d]=t:=DllCall("GetDriveType","Str",(f:=Chr(A_Index+64)) ":\"))||(d="all"&&t!=1))
l.=f
}
return l
}