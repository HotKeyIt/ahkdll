A_WinDir(b:=0,n:=0){
static d,i:=VarSetCapacity(d,520) DllCall("GetWindowsDirectory",PTR,&d,UInt,260) VarSetCapacity(d,-1) d:=SubStr(d,-1)="\"?SubStr(d,1,-1):d,l:=StrLen(d)
Critical
return b?StrPut(d,b):n?l:d
}