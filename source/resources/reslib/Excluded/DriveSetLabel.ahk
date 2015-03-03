DriveSetLabel(d,l){
DllCall("SetErrorMode","Int",1)
ErrorLevel:=!DllCall("SetVolumeLabel","Str",d (SubStr(d,-1)!="\"?"\":""),"Str",l)
}