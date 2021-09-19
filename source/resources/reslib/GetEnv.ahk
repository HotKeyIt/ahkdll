GetEnv(){
  allEnvStr:={}
  _EnvStr:=EnvStr:=DllCall("GetEnvironmentStringsW","PTR")
  While thisEnvStr:=SubStr(temp:=StrGet(EnvStr,"UTF-16"),1,InStr(temp,"=",1,2)-1)
    If ((EnvStr+=StrLen(temp)*2+2) && RegExMatch(thisEnvStr,"^[\w_]+$"))
      allEnvStr.%thisEnvStr%:=EnvGet(thisEnvStr)
  return (DllCall("FreeEnvironmentStringsW","PTR",_EnvStr), allEnvStr)
}