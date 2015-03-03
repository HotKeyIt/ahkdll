EnvSet(n,v:=""){
ErrorLevel:=DllCall("SetEnvironmentVariable","Str",n,"Str",v)?0:1
}