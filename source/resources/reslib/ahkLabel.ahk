ahkLabel(label, DoNotWait:=0){
static ahkLabel
if !ahkLabel
ahkLabel:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkLabel","PTR"),"i==sui")
Errorlevel := ahkLabel[label,DoNotWait]
}