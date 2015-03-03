DirCopy(szInputSource, szInputDest, bOverwrite:=0){
	static szSource,szDest,init:=VarSetCapacity(szSource,522) VarSetCapacity(szDest,522)
        ,SHFILEOPSTRUCT := "HWND hwnd;UINT wFunc;PCTSTR pFrom;PCTSTR pTo;UINT fFlags;BOOL fAnyOperationsAborted;LPVOID hNameMappings;PCTSTR lpszProgressTitle"
        ,FileOp:=Struct(SHFILEOPSTRUCT),FO_COPY:=2,FOF_SILENT:=4,FOF_NOCONFIRMMKDIR:=512, FOF_NOCONFIRMATION:=16,FOF_NOERRORUI:=1024
  ; Get the fullpathnames and strip trailing \s
	szSource := GetFullPathName(szInputSource)
	szDest := GetFullPathName(szInputDest)

	; Ensure source is a directory
	if !DirExist(szSource){
		ErrorLevel := 1
		return							; Nope
	}
	; Does the destination dir exist?
	if DirExist(szDest)&&!bOverwrite{
		ErrorLevel:=1
		return
	}	else if !DirCreate(szDest,error)&&error { ; Although dest doesn't exist as a dir, it might be a file, which is covered below too.
		; We must create the top level directory
		; Failure is expected to happen if szDest is an existing *file*, since a dir should never be allowed to overwrite a file (to avoid accidental loss of data).
		Errorlevel:=1
    return
	}

	; To work under old versions AND new version of shell32.dll the source must be specified
	; as "dir\*.*" and the destination directory must already exist... Goddamn Microsoft and their APIs...
  szSource.="\*.*"

	; We must also make source\dest double nulled strings for the SHFileOp API
  NumPut(0,szSource,StrLen(szSource)*2+2,"Short")
  NumPut(0,szDest,StrLen(szDest)*2+2,"Short")	

	; Setup the struct
	FileOp.Fill()
	FileOp.pFrom := &szSource
	FileOp.pTo := &szDest
	FileOp.wFunc := FO_COPY
	FileOp.fFlags := FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI ; FOF_NO_UI ("perform the operation with no user input") is not present for in case it would break compatibility somehow, and because the other flags already present seem to make its behavior implicit.  Also, unlike FileMoveDir, FOF_MULTIDESTFILES never seems to be needed.
	; All of the below left set to NULL/FALSE by the struct initializer higher above:
	;FileOp.hNameMappings			= NULL
	;FileOp.lpszProgressTitle		= NULL
	;FileOp.fAnyOperationsAborted	= FALSE
	;FileOp.hwnd					= NULL

	; If the source directory contains any saved webpages consisting of a SiteName.htm file and a
	; corresponding directory named SiteName_files, the following may indicate an error even when the
	; copy is successful. Under Windows XP at least, the return value is 7 under these conditions,
	; which according to WinError.h is "ERROR_ARENA_TRASHED: The storage control blocks were destroyed."
	; However, since this error might occur under a variety of circumstances, it probably wouldn't be
	; proper to consider it a non-error.
	; I also checked GetLastError() after calling SHFileOperation(), but it does not appear to be
	; valid/useful in this case (MSDN mentions this fact but isn't clear about it).
	; The issue appears to affect only FileCopyDir, not FileMoveDir or FileRemoveDir.  It also seems
	; unlikely to affect FileCopy/FileMove because they never copy directories.
	ErrorLevel:=!!DllCall("shell32\SHFileOperationW","PTR",FileOp[])
}