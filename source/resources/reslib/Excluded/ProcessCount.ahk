ProcessCount(){
	static proc,init:=VarSetCapacity(proc,1024*4)
	If InStr( ",WIN_NT4,WIN_95,WIN_98,WIN_ME,WIN_XP,WIN_2003," , "," A_OSVersion ",")
		DllCall("psapi\EnumProcesses","PTR",&proc,"UInt",1024,"UInt*",pBytesReturned)
	else DllCall("K32EnumProcesses","PTR",&proc,"UInt",1024,"UInt*",pBytesReturned)
	return pBytesReturned//4
}