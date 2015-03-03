A_ModulePath(b:=0,n:=0){
static d,l:=VarSetCapacity(d,520),i:=(l:=DllCall("GetModuleFileNameW","PTR",A_ModuleHandle,"PTR",&d,"UInt",260)) ? VarSetCapacity(d,-1) : (l:=DllCall("GetModuleFileNameW","PTR",0,"PTR",&d,"UInt",260))?VarSetCapacity(d,-1):1
Critical
return b?StrPut(d,b):n?l:d
}