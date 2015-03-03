A_UserName(b:=0,n:=0){
static d,i:=VarSetCapacity(d,520,0),G:=DynaCall("advapi32\GetUserNameW","tt")
Critical
If !G[&d,getvar(len:=260)]
StrPut("",&d)
else VarSetCapacity(d,-1)
return b?StrPut(d,b):n?260:d
}