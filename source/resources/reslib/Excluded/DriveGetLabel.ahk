DriveGetLabel(d){
static v,i:=VarSetCapacity(v,522)
DllCall("SetErrorMode","Int",1)
if !DllCall("GetVolumeInformation","Str",d (SubStr(d,-1)!="\"?"\":""),"PTR",&v,"Int",260,"UInt*",0,"UInt*",0,"UInt*",0,"PTR",0,"Int",0)
return -1
else VarSetCapacity(v,-1)
return v
}