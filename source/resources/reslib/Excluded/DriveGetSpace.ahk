DriveGetSpace(d){
static MB:=1024*1024
if (!d)
return -1
DllCall("SetErrorMode","Int",1)
if !DllCall("GetDiskFreeSpaceEx","Str",d (SubStr(d,-1)!="\"?"\":""),"INT64*",f,"PTR", 0,"PTR",0)
return -1
return f/MB
}
