DriveGetType(d){
static s:={5:"CDROM",2:"REMOVABLE",3:"FIXED",4:"REMOTE",6:"RAMDISK",0:"UNKNOWN"}
DllCall("SetErrorMode","Int",1)
return s[DllCall("GetDriveType","Str",d (SubStr(d,-1)!="\"?"\":""))]
}