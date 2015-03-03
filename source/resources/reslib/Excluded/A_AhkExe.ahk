A_AhkExe(b:=0,n:=0){
static e,i:=VarSetCapacity(e,520) DllCall("GetModuleFileName",PTR,0,Str,e,UInt,260) VarSetCapacity(e,-1),l:=StrLen(e)
Critical
return b?StrPut(e,b):n?l:e
}