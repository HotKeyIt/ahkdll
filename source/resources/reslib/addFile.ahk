addFile(Script,waitExecute:=0){
global
static addFile:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"addFile"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","addFile","PTR"),"i==si" (!A_IsDll?"ui":""))
return addFile[Script,waitExecute]
}