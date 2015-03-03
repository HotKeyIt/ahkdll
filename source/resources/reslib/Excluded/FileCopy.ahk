FileCopy(szInputSource, szInputDest:="", bOverwrite:=0){
	static szDrive:=Struct("TCHAR[261]"),szDir:=Struct("TCHAR[261]"),szExpandedDest:=Struct("TCHAR[261]")
				,PM_NOREMOVE:=0,msg:=Struct("HWND hwnd;UINT message;WPARAM wParam;LPARAM lParam;DWORD time;int x;int y"),FILE_ATTRIBUTE_DIRECTORY:=16
				,FILETIME:="DWORD dwLowDateTime;DWORD dwHighDateTime"
				,WIN32_FIND_DATA:="DWORD dwFileAttributes;FileCopy(FILETIME) ftCreationTime;FileCopy(FILETIME) ftLastAccessTime;FileCopy(FILETIME) ftLastWriteTime;DWORD nFileSizeHigh;DWORD nFileSizeLow;DWORD dwReserved0;DWORD dwReserved1;TCHAR cFileName[260];TCHAR cAlternateFileName[14]"
				,findData := Struct(WIN32_FIND_DATA),g:=GlobalStruct(),g_script:=ScriptStruct(),ERROR_BUFFER_OVERFLOW:=111,_tsplitpath:=DynaCall("msvcrt\_wsplitpath","i=ttttt")
				,szFileTemp:=Struct("TCHAR[261]"),szExtTemp:=Struct("TCHAR[261]"),szSrcFile:=Struct("TCHAR[261]"),szSrcExt:=Struct("TCHAR[261]")
        ,szDestDrive:=Struct("TCHAR[261]"),szDestDir:=Struct("TCHAR[261]"),szDestFile:=Struct("TCHAR[261]"),szDestExt:=Struct("TCHAR[261]")
        ,_tcscpy:=DynaCall("msvcrt\wcscpy","t==tt"),_tcscat:=DynaCall("msvcrt\wcscat","t==tt")
        ,_tcschr:=DynaCall("msvcrt\wcschr","i==ti")
        ,szSource,szDest,init:=VarSetCapacity(szSource,520) VarSetCapacity(szDest,520)

	; Get local version of our source/dest with full path names, strip trailing \s
	szSource:=GetFullPathName(szInputSource),szDest:=szInputDest=""?A_LoopFileFullPath:GetFullPathName(szInputDest)

	; If the source or dest is a directory then add *.* to the end
	if DirExist(szSource)
		szSource.="\*.*"
	if DirExist(szDest)
		szDest.="\*.*"
	If -1 = hSearch := DllCall("FindFirstFile","Str",szSource,"PTR",findData[],"PTR"){
		ErrorLevel:=1
		return ; Indicate no failures.
	}
	; aLastError := 0 ; Set default. Overridden only when a failure occurs.

	; Otherwise, loop through all the matching files.
	; Split source into file and extension (we need this info in the loop below to reconstruct the path)
	_tsplitpath[&szSource, szDrive[],szDir[],0,0]
	
	; Note we now rely on the SOURCE being the contents of szDrive, szDir, szFile, etc.
	szTempPath_length := StrLen(szTempPath := StrGet(szDrive[]) StrGet(szDir[]))
	append_pos := &szTempPath + (szTempPath_length*2)
	space_remaining := 261 - szTempPath_length - 1
	
	failure_count := 0
	
	Loop{
		; Since other script threads can interrupt during LONG_OPERATION_UPDATE, it's important that
		; this function and those that call it not refer to sArgDeref[] and sArgVar[] anytime after an
		; interruption becomes possible. This is because an interrupting thread usually changes the
		; values to something inappropriate for this thread.
		tick_now := A_TickCount
		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", PM_NOREMOVE)
				Sleep -1
			tick_now := A_TickCount
			g_script.mLastPeekTime := tick_now
		}

		; Make sure the returned handle is a file and not a directory before we
		; try and do copy type things on it!
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ; dwFileAttributes should never be invalid (0xFFFFFFFF) in this case.
			continue

		if (StrLen(StrGet(findData.cFileName[""])) > space_remaining) ; v1.0.45.03: Basic check in case of files whose full spec is over 260 characters long.
		{
			; aLastError := ERROR_BUFFER_OVERFLOW ; MSDN: "The file name is too long."
			++failure_count
			continue
		}
		_tcscpy[append_pos, findData.cFileName[""]] ; Indirectly populate szTempPath. Above has ensured this won't overflow.
		


		; Expand the destination based on this found file
		; Util_ExpandFilenameWildcard(findData.cFileName, szDest, szExpandedDest)
		if (_tcschr[&szDest, Ord("*")] == 0){
			_tcscpy[szExpandedDest[], &szDest]
		} else {

			; Split source and dest into file and extension
			_tsplitpath[ findData.cFileName[""], szDestDrive[], szDestDir[], szSrcFile[], szSrcExt[] ]
			_tsplitpath[ &szDest, szDestDrive[], szDestDir[], szDestFile[], szDestExt[] ]

			; Source and Dest ext will either be ".nnnn" or "" or ".*", remove the period
			if (szSrcExt.1 == ".")
				_tcscpy[szSrcExt[], szSrcExt.2[""]]
			if (szDestExt.1 == ".")
				_tcscpy[szDestExt[], szDestExt.2[""]]

			; Start of the destination with the drive and dir
			_tcscpy[szExpandedDest[], szDestDrive[]]
			_tcscat[szExpandedDest[], szDestDir[]]
			
			
			
			; Replace first * in the destext with the srcext, remove any other *
			; Util_ExpandFilenameWildcardPart(szSrcExt, szDestExt, szExtTemp)
			; Replace first * in the dest with the src, remove any other *
			i := 1, j := 1, k := 1
			lpTemp := _tcschr[szDestExt[], Ord("*")]
			if (lpTemp != 0){
				; Contains at least one *, copy up to this point
				while(szDestExt[i] != "*")
					szExtTemp[j++] := szDestExt[i++]
				; Skip the * and replace in the dest with the srcext
				while(szSrcExt[k] != "")
					szExtTemp[j++] := szSrcExt[k++]
				; Skip any other *
				i++
				while(szDestExt[i] != "")
					if (szDestExt[i] == "*")
						i++
					else	szExtTemp[j++] := szDestExt[i++]
				szExtTemp[j] := ""
			}	else	{
				; No wildcard, straight copy of destext
				_tcscpy[szExtTemp[], szDestExt[]]
			}
		}
		
		; Replace first * in the destfile with the srcfile, remove any other *
		; Util_ExpandFilenameWildcardPart(szSrcFile, szDestFile, szFileTemp)
		; Replace first * in the dest with the src, remove any other *
		i := 1, j := 1, k := 1
		lpTemp := _tcschr[szDestFile[], Ord("*")]
		if (lpTemp != 0)
		{
			; Contains at least one *, copy up to this point
			while(szDestFile[i] != "*")
				szFileTemp[j++] := szDestFile[i++]
			; Skip the * and replace in the dest with the srcext
			while(szSrcFile[k] != "")
				szFileTemp[j++] := szSrcFile[k++]
			; Skip any other *
			i++
			while(szDestFile[i] != "")
			{
				if (szDestFile[i] == "*")
					i++
				else
					szFileTemp[j++] := szDestFile[i++]
			}
			szFileTemp[j] := ""
		}
		else
		{
			; No wildcard, straight copy of destext
			_tcscpy[szFileTemp[], szDestFile[]]
		}
		
		; Concat the filename and extension if req
		if (szExtTemp[1] != "")
		{
			_tcscat[szFileTemp[], &"."]
			_tcscat[szFileTemp[], szExtTemp[]]
		}
		else
		{
			; Dest extension was blank SOURCE MIGHT NOT HAVE BEEN!
			if (szSrcExt[1] != "")
			{
				_tcscat[szFileTemp[], &"."]
				_tcscat[szFileTemp[], szSrcExt[]]	
			}
		}
		; Now add the drive and directory bit back onto the dest
		_tcscat[szExpandedDest[], szFileTemp[]]
		
		; Fixed for v1.0.36.01: This section has been revised to avoid unnecessary calls but more
		; importantly, it now avoids the deletion and complete loss of a file when it is copied or
		; moved onto itself.  That used to happen because any existing destination file used to be
		; deleted prior to attempting the move/copy.
		if (!DllCall("CopyFile","Str",szTempPath,"PTR", szExpandedDest[],"Char", !bOverwrite)) ; Force it to fail if bOverwrite==false.
		{
			; aLastError := A_LastError
			++failure_count
		}
	} Until !DllCall("FindNextFile","PTR",hSearch,"PTR",findData[])

	DllCall("FindClose","PTR",hSearch)
	ErrorLevel:=failure_count
	return
}