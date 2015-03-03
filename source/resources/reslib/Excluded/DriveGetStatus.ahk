DriveGetStatus(d){
static s:={0:"Ready",3:"Invalid",64:"NotAvailable",21:"NotReady",19:"ReadOnly",53:"NotAvailable"}
DllCall("SetErrorMode","Int",1)
return s[DllCall("GetDiskFreeSpaceEx","Str",d (SubStr(d,-1)!="\"?"\":""),"UInt*",0,"UInt*",0,"UInt*",0,"UInt*",0)?0:A_LastError]
}