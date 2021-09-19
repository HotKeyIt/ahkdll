ahkExecuteline(pLine:=0,Mode:=0,Wait:=1){
static _ahkExecuteLine:=DynaCall(A_IsDll&&A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkExecuteLine"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkExecuteLine","PTR"),"t==tuiuiui",0,0,0,GetCurrentThreadId())
return _ahkExecuteLine(pLine,Mode,Wait)
}