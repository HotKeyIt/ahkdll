addScript(Script,waitExecute:=0){
static _addScript:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"addScript"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","addScript","PTR"),"i==siui","",0,GetCurrentThreadId())
return _addScript(Script,waitExecute)
}