DriveGetSerial(d){
DllCall("SetErrorMode","Int",1)
if !DllCall("GetVolumeInformation","Str",d (SubStr(d,-1)!="\"?"\":""),"PTR",0,"Int",0,"UInt*",s,"UInt*",0,"UInt*",0,"PTR",0,"Int",0)
return -1
return s
}