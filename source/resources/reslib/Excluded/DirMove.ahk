DirMove(szInputSource, szInputDest, OverwriteMode:=0){
  static szSource,szDest,i:=VarSetCapacity(szSource,522) VarSetCapacity(szSource,522)
        ,SHFILEOPSTRUCT := "HWND hwnd;UINT wFunc;PCTSTR pFrom;PCTSTR pTo;UINT fFlags;BOOL fAnyOperationsAborted;LPVOID hNameMappings;PCTSTR lpszProgressTitle"
        ,FileOp:=Struct(SHFILEOPSTRUCT),FO_MOVE:=1,FOF_SILENT:=4,FOF_NOCONFIRMMKDIR:=512, FOF_NOCONFIRMATION:=16,FOF_NOERRORUI:=1024,FOF_MULTIDESTFILES:=1
	; Get the fullpathnames and strip trailing \s
	szSource:=GetFullPathName(szInputSource),szDest:=GetFullPathName(szInputDest)

	; Ensure source is a directory
	if !DirExist(szSource){
		ErrorLevel:=1
		return							; Nope
	}

	; Does the destination dir exist?
	attr := DllCall("GetFileAttributes","Str",szDest,"UInt")
	if (attr != 0xFFFFFFFF) ; Destination already exists as a file or directory.
	{
		if (attr & 16) ; Dest already exists as a directory, FILE_ATTRIBUTE_DIRECTORY.
		{
			if (OverwriteMode != 1 && OverwriteMode != 2){ ; Overwrite Mode is "Never".  Strict validation for safety.
				ErrorLevel:=1
      return ; For consistency, mode1 actually should move the source-dir *into* the identically name dest dir.  But for backward compatibility, this change hasn't been made.
		}	else { ; Dest already exists as a file.
			ErrorLevel:=1
      return false ; Don't even attempt to overwrite a file with a dir, regardless of mode (I think SHFileOperation refuses to do it anyway).
    }
	}

	if IsDifferentVolumes(szSource, szDest){
		; If the source and dest are on different volumes then we must copy rather than move
		; as move in this case only works on some OSes.  Copy and delete (poor man's move).
		if !DirCopy(szSource, szDest, true){
      ErrorLevel:=1
			return
    }
    ErrorLevel:=DirDelete(szSource, true)
		return
	}

	; Since above didn't return, source and dest are on same volume.
	; We must also make source\dest double nulled strings for the SHFileOp API
  NumPut(0,szSource,StrLen(szSource)*2+2,"Short")
  NumPut(0,szDest,StrLen(szSource)*2+2,"Short")

	; Setup the struct
	FileOp.Fill()
	FileOp.pFrom := &szSource
	FileOp.pTo := &szDest
	FileOp.wFunc := FO_MOVE
	FileOp.fFlags := FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI ; Set default. FOF_NO_UI ("perform the operation with no user input") is not present for in case it would break compatibility somehow, and because the other flags already present seem to make its behavior implicit.
	if (OverwriteMode = 2) ; v1.0.46.07: Using the FOF_MULTIDESTFILES flag (as hinted by MSDN) overwrites/merges any existing target directory.  This logic supersedes and fixes old logic that didn't work properly when the source dir was being both renamed and moved to overwrite an existing directory.
		FileOp.fFlags |= FOF_MULTIDESTFILES
	; All of the below left set to NULL/FALSE by the struct initializer higher above:
	;FileOp.hNameMappings			= NULL
	;FileOp.lpszProgressTitle		= NULL
	;FileOp.fAnyOperationsAborted	= FALSE
	;FileOp.hwnd					= NULL
  ErrorLevel:=!!DllCall("shell32\SHFileOperationW","PTR",FileOp[])
}