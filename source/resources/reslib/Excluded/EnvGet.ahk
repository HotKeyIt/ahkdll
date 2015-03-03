EnvGet(aEnvVarName){
static buf,init:=VarSetCapacity(buf,32767*2)
if (length := DllCall("GetEnvironmentVariable","Str",aEnvVarName,"PTR", &buf[],"Int", 32767*2))
return StrGet(&buf)
}
