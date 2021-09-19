errormessage(E:=0){ 
static ES:=BufferAlloc(1024)
FormatMessage(0x00001000,0,e?e:A_LastError,0,ES.Ptr,1024)
return StrReplace(StrGet(ES),"`r`n"," ")
}