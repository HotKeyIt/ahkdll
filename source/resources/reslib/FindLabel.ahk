FindLabel(Name){
return DllCall(A_MemoryModule?MemoryGetProcAddress(A_MemoryModule,"ahkFindLabel"):DllCall("GetProcAddress","PTR",A_ModuleHandle,"AStr","ahkFindLabel","PTR"	),"Str",Name,"CDecl PTR")
}