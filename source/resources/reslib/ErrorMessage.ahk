errormessage(E:=0){ 
static es,i:=VarSetCapacity(ES,1024),s:=A_Space
DllCall("FormatMessage",UINT,0x00001000,UINT,0,UINT,e?e:A_LastError,UINT,0,Str,ES,UINT,1024,str,"")
return StrReplace(ES,"`r`n",s)
}