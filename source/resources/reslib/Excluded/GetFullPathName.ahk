GetFullPathName(szIn){
static szOut,init:=VarSetCapacity(szOut,520)
if DllCall("GetFullPathName","STR",szIn,"UInt", 260,"Str", szOut,"PTR",0)
return trim(szOut,"\/")
}