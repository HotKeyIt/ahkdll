ahkExec(Script){
static _ahkExec:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkExec"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkExec","PTR"),"i==sui","",GetCurrentThreadId())
return _ahkExec(Script)
}