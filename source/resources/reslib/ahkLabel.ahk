ahkLabel(label, DoNotWait:=0){
static ahkLabel:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkLabel","PTR"),"i==sui" (!A_IsDll?"ui":""))
return ahkLabel[label,DoNotWait]
}