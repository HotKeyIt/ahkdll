A_AhkDir(b:=0,n:=0){
static d,i:=VarSetCapacity(d,520) DllCall("GetModuleFileName",PTR,0,Str,d,UInt,260) VarSetCapacity(d,-1) d:=SubStr(d,1,InStr(d,"\",1,-1)-1),l:=StrLen(d)
Critical
return b?StrPut(d,b):n?l:d
}