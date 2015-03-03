DirDelete(szInputSource,bRecurse:=0){
	static szSource,i:=VarSetCapacity(szSource,524)
        ,SHFILEOPSTRUCT := "HWND hwnd;UINT wFunc;PCTSTR pFrom;PCTSTR pTo;UINT fFlags;BOOL fAnyOperationsAborted;LPVOID hNameMappings;PCTSTR lpszProgressTitle"
        ,FileOp:=Struct(SHFILEOPSTRUCT),FO_DELETE:=3,FOF_SILENT:=4,FOF_NOCONFIRMMKDIR:=512, FOF_NOCONFIRMATION:=16,FOF_NOERRORUI:=1024
	; Get the fullpathnames and strip trailing \s
	szSource:=GetFullPathName(szInputSource)
	; Ensure source is a directory
	if !DirExist(szSource){
		ErrorLevel:=1							; Nope
    return
  }
	; If recursion not on just try a standard delete on the directory (the SHFile function WILL
	; delete a directory even if not empty no matter what flags you give it...)
	if !bRecurse {
    ErrorLevel:=!DllCall("RemoveDirectory","Str",szSource)
		return
  }
	; We must also make double nulled strings for the SHFileOp API
  NumPut(0,szSource,StrLen(szSource)*2+2,"Short")
	; Setup the struct
	FileOp.pFrom := &szSource
	FileOp.pTo := 0
	FileOp.hNameMappings := 0
	FileOp.lpszProgressTitle := 0
	FileOp.fAnyOperationsAborted := 0
	FileOp.hwnd := 0
	FileOp.wFunc	:= FO_DELETE
	FileOp.fFlags	:= FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI
	ErrorLevel:=!!DllCall("shell32\SHFileOperationW","PTR",FileOp[])
}