ahkExec(Script){
global
static ahkExec:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkExec"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkExec","PTR"),"i==s" (!A_IsDll?"ui":""))
Errorlevel := ahkExec[Script]
}