DriveGetFileSystem(d){
static f,i:=VarSetCapacity(f,522)
DllCall("SetErrorMode","Int",1)
if !DllCall("GetVolumeInformation","Str",d (SubStr(d,-1)!="\"?"\":""),"PTR",0,"Int",0,"UInt*",0,"UInt*",0,"UInt*",0,"PTR",&f,"Int",256)
return -1
else VarSetCapacity(f,-1)
return f
}