A_Temp(b:=0,n:=0){
static d,i:=VarSetCapacity(d,520),g:=DynaCall("GetTempPath",["uit",2],260,&d)
Critical
return b&&G[]&&VarSetCapacity(d,-1)?StrPut(rtrim(d,"\"),b):n?G[]:G[]&&VarSetCapacity(d,-1)?rtrim(d,"\"):""
}
