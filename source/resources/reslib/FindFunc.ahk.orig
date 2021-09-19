FindFunc(Name, thread:=0){
return DllCall(A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkFindFunc"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkFindFunc","PTR"),"Str",Name,"Uint",thread,"CDecl PTR")
}