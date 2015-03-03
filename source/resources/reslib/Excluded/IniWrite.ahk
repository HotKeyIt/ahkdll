IniWrite(aValue, aFilespec, aSection:="", aKey:=""){
	static GENERIC_WRITE:=1073741824,CREATE_NEW:=1,BOM,Init:=VarSetCapacity(BOM,4) NumPut(0xFEFF,BOM,"UInt")
	; Get the fullpathname (INI functions need a full path) 
	szFileTemp:=GetFullPathName(aFilespec)
	; WritePrivateProfileStringW() always creates INIs using the system codepage.
	; IniEncodingFix() checks if the file exists and if it doesn't then it creates
	; an empty file with a UTF-16LE BOM.
	result := TRUE
	if (!FileExist(szFileTemp))
	{
		; Create a Unicode file. (UTF-16LE BOM)
		if -1 != hFile := DllCall("CreateFile","Str",szFileTemp,"UInt", GENERIC_WRITE,"UInt", 0,"PTR", 0,"UInt", CREATE_NEW,"UInt", 0,"PTR", NULL){

			; Write a UTF-16LE BOM to identify this as a Unicode file.
			; Write [%aSection%] to prevent WritePrivateProfileString from inserting an empty line (for consistency and style).
			if (   !DllCall("WriteFile","PTR",hFile,"PTR",&BOM,"UInt", 2,"UInt*",dwWritten,"PTR",0) || dwWritten != 2
				|| !DllCall("WriteFile","PTR",hFile,"Str", "[" aSection "]","UInt", cb:=(Strlen(aSection) + 2) * 2,"UInt*", dwWritten,"PTR",0) || dwWritten != cb   )
				result := FALSE

			if !DllCall("CloseHandle","PTR",hFile)
				result := FALSE
		}
	}
	if(result){
		if (aKey!="")
			result := DllCall("WritePrivateProfileString","Str",aSection,"Str",aKey,"Str",aValue,"Str",szFileTemp)  ; Returns zero on failure.
		else
		{
			szBuffer:=Struct("Ushort",StrLen(aValue)+1),StrPut(aValue,szBuffer[])
			; Convert newline-delimited list to null-terminated array of null-terminated strings.
			Loop % StrLen(aValue)
				if A_LoopField="`n"
					szBuffer[i]:=0
			result := DllCall("WritePrivateProfileSection","Str",aSection,"PTR",szBuffer[],"Str",szFileTemp)
		}
		DllCall("WritePrivateProfileString","PTR",0,"PTR",0,"PTR",0,"Str",szFileTemp)	; Flush
	}
	ErrorLevel:=!result
	return
}
