GetEnv(){
  global
  local __env__:=DllCall("GetEnvironmentStrings","PTR"),__thisenv__,__temp__
  While __thisenv__:=SubStr(__temp__:=StrGet(__env__,"CP0"),1,InStr(__temp__,"=",1,2)-1)
    If ((__env__+=StrLen(__temp__)+1) && RegExMatch(__thisenv__,"^[\w_]+$"))
      EnvGet,%__thisenv__%,% __thisenv__
}