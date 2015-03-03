SetWorkingDir(d){
ErrorLevel:=DllCall("SetCurrentDirectory","Str",d)?0:1
}