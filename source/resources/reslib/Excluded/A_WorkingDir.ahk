A_WorkingDir(b:=0,n:=0){
static d,i:=VarSetCapacity(d,520),G:=DynaCall("GetCurrentDirectory","uit")
Critical
l:=G[260,&d],VarSetCapacity(d,-1)
return b?StrPut(d,b):n?l:d
}
A_WorkingDirSet(b:=0,n:=0){
Critical
return DllCall("SetCurrentDirectory",PTR,b),1
}