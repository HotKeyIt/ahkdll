FindLabel(Name,thread:=0){
static ahkFindLabel:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkFindLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkFindLabel","PTR"),"i==sui","",GetCurrentThreadId())
return ahkFindLabel(Name)
}