addScript(Script,waitExecute:=0){
global
static addScript
if !addScript
addScript:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"addScript"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","addScript","PTR"),"i==si")
return addScript[Script,waitExecute]
}