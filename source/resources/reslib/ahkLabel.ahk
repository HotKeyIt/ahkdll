ahkLabel(label, DoNotWait:=0){
static _ahkLabel:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkLabel","PTR"),"i==suiui","",0,GetCurrentThreadId())
return _ahkLabel(label,DoNotWait)
}