A_IsDll(b:=0,n:=0){
Critical
return b?StrPut(A_ModuleHandle!=DllCall("GetModuleHandle","PTR",0,"PTR"),b):n?1:A_ModuleHandle!=DllCall("GetModuleHandle","PTR",0,"PTR")
}