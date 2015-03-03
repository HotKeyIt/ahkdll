IniDelete(aFilespec,aSection,aKey){
; Note that aKey can be NULL, in which case the entire section will be deleted.
	szFileTemp:=GetFullPathName(aFilespec)
	; Get the fullpathname (ini functions need a full path) 
	ErrorLevel:= !result := DllCall("WritePrivateProfileString","Str",aSection,"Str",aKey,"PTR",0,"Str", szFileTemp)  ; Returns zero on failure.
	,DllCall("WritePrivateProfileString","PTR",0,"PTR",0,"PTR",0,"Str",szFileTemp)	; Flush
	return
}