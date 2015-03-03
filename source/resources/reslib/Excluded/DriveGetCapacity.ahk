DriveGetCapacity(d){
static MB:=1024*1024
if (!d)
return -1
DllCall("SetErrorMode","Int",1)
if !DllCall("GetDiskFreeSpaceEx","Str",d (SubStr(d,-1)!="\"?"\":""),"PTR",0,"INT64*", t,"PTR",0)
return -1
return t/MB
}