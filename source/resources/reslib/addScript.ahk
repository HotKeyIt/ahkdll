addScript(Script,waitExecute:=0){
static ahkExec:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"addScript"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","addScript","PTR"),"i==si")
return ahkExec[Script,waitExecute]
}