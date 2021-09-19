FindLabel(Name,thread:=0){
return DllCall(A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkFindLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkFindLabel","PTR"),"Str",Name,"Uint",thread,"CDecl PTR")
}