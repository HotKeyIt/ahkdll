FindFunc(Name, thread:=0){
static ahkFindFunc:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkFindFunc"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkFindFunc","PTR"),"i==sui","",GetCurrentThreadId())
return ahkFindFunc(Name)
}