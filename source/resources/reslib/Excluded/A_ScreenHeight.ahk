A_ScreenHeight(b:=0,n:=0){
static g:=DynaCall("GetSystemMetrics","ui",1)
Critical
return b?StrPut(G[],b):n?20:G[]
}