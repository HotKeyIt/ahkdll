errormessage(E:=0){ 
     if !E
          E:=A_LastError
     VarSetCapacity(ES, 1024),DllCall("FormatMessage",UINT,0x00001000,UINT,0,UINT,e,UINT,0,Str,ES,UINT,1024,str,"")
     return StrReplace(ES,"`r`n",A_Space)
}