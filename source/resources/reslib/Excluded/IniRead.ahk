IniRead(aFilespec, aSection:="", aKey:="", aDefault:=""){
	static szBuffer:=Struct("UShort[65536]")					; Max ini file size is 65535 under 95
	; Get the fullpathname (ini functions need a full path):
	szFileTemp:=GetFullPathName(aFilespec)
	if (aKey!="")
	{
		; An access violation can occur if the following conditions are met:
		;	1) aFilespec specifies a Unicode file.
		;	2) aSection is a read-only string, either empty or containing only spaces.
		;
		; Debugging at the assembly level indicates that in this specific situation,
		; it tries to write a zero at the end of aSection (which is already null-
		; terminated).
		;
		; The second condition can ordinarily only be met if Section is omitted,
		; since in all other cases aSection is writable.  Although Section was a
		; required parameter prior to revision 57, empty or blank section names
		; are actually valid.  Simply passing an empty writable buffer appears
		; to work around the problem effectively:
		DllCall("GetPrivateProfileString","Str",aSection,"Str",aKey,"Str", aDefault,"Ptr",szBuffer[],"Uint",65535,"Str",szFileTemp)
	}
	else if (aSection!=""
		? DllCall("GetPrivateProfileSection","Str",aSection,"Ptr",szBuffer[],"UInt",65536,"Str",szFileTemp)
		: DllCall("GetPrivateProfileSectionNames","PTR",szBuffer[],"UInt",65536,"Str",szFileTemp))
	{
		; Convert null-terminated array of null-terminated strings to newline-delimited list.
		Loop 65536
			If !szBuffer[A_Index]{
				if (!szBuffer[A_Index+1])
					break
				szBuffer[A_Index] := 10 ; `n
			}
	}
	; If the value exists but is empty, the return value and GetLastError() will both be 0,
	; so assign ErrorLevel solely based on GetLastError():
	ErrorLevel:=!!A_LastError
	; The above function is supposed to set szBuffer to be aDefault if it can't find the
	; file, section, or key.  In other words, it always changes the contents of szBuffer.
	return StrGet(szBuffer[]) ; Avoid using the length the API reported because it might be inaccurate if the data contains any binary zeroes, or if the data is double-terminated, etc.
	; Note: ErrorLevel is not changed by this command since the aDefault value is returned
	; whenever there's an error.
}