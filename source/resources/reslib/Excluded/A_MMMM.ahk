A_MMMM(b:=0,n:=0){
static i:=VarSetCapacity(t,999),G:=DynaCall("GetDateFormatW",["uiuitsti",5,6],1024,0,0,"MMMM")
Critical
return b?G[b,b?999:0]-1:n?G[]-1:G[&t,999] && VarSetCapacity(t,-1)?t:""
}