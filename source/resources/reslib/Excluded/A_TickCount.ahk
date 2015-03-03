A_TickCount(b:=0,n:=0){
static T:=DynaCall("GetTickCount","ui=")
Critical
return b?StrPut(T[],b):n?20:T[]
}