FileDelete(aFilePattern){
	static MAX_PATH:=260,PM_NOREMOVE:=0,ERROR_BUFFER_OVERFLOW:=111,FILE_ATTRIBUTE_DIRECTORY:=16,FILETIME:="DWORD dwLowDateTime;DWORD dwHighDateTime",
				,WIN32_FIND_DATA:="DWORD dwFileAttributes;FileDelete(FILETIME) ftCreationTime;FileDelete(FILETIME) ftLastAccessTime;FileDelete(FILETIME) ftLastWriteTime;DWORD nFileSizeHigh;DWORD nFileSizeLow;DWORD dwReserved0;DWORD dwReserved1;TCHAR cFileName[260];TCHAR cAlternateFileName[14]"
				,current_file:=Struct(WIN32_FIND_DATA)
				,g:=GlobalStruct(),g_script:=ScriptStruct()
	; Below is done directly this way rather than passed in as args mainly to emphasize that
	; ArgLength() can safely be called in Line methods like this one (which is done further below).
	; It also may also slightly improve performance and reduce code size.

	if (aFilePattern="")
	{
		; Let ErrorLevel indicate an error, since this is probably not what the user intended.
		ErrorLevel:=1
    return
	}

	if !InStr(aFilePattern,"?") && !InStr(aFilePattern,"*")	{
		DllCall("SetLastError","UInt",0) ; For sanity: DeleteFile appears to set it only on failure.
		; ErrorLevel will indicate failure if DeleteFile fails.
		ErrorLevel:=!DllCall("DeleteFile","Str",aFilePattern)
    return
	}

	; Otherwise aFilePattern contains wildcards, so we'll search for all matches and delete them.
	; Testing shows that the ANSI version of FindFirstFile() will not accept a path+pattern longer
	; than 256 or so, even if the pattern would match files whose names are short enough to be legal.
	; Therefore, as of v1.0.25, there is also a hard limit of MAX_PATH on all these variables.
	; MSDN confirms this in a vague way: "In the ANSI version of FindFirstFile(), [plpFileName] is
	; limited to MAX_PATH characters."
	if (StrLen(aFilePattern) >= MAX_PATH) ; Checked early to simplify things later below.
	{
		; Let ErrorLevel indicate the error.
		ErrorLevel:=1
		return
	}

	
	failure_count := 0 ; Set default.

	if -1 = file_search := DllCall("FindFirstFile","Str",aFilePattern,"PTR", current_file[],"PTR"){
	 ; No matching files found.
		g.LastError := DllCall("GetLastError") ; Probably (but not necessarily) ERROR_FILE_NOT_FOUND.
		ErrorLevel:=0
		return ; Deleting a wildcard pattern that matches zero files is a success.
	}

	VarSetCapacity(file_path,520)
	StrPut(aFilePattern,&file_path)
	; Remove the filename and/or wildcard part.   But leave the trailing backslash on it for
	; consistency with below:
	if (last_backslash := InStr(file_path, "\",1,-1))
	{
		NumPut(0,&file_path,last_backslash*2+2,"Short") ;*(last_backslash + 1) = '\0' ; i.e. retain the trailing backslash.
		file_path_length := StrLen(file_path)
	}
	else ; Use current working directory, e.g. if user specified only *.*
	{
		file_path := ""
		file_path_length := 0
	}

	append_pos := &file_path + file_path_length*2 ; For performance, copy in the unchanging part only once.  This is where the changing part gets appended.
	space_remaining := StrLen(file_path) - file_path_length - 1 ; Space left in file_path for the changing part.

	g.LastError := 0 ; Set default. Overridden only when a failure occurs.
	Loop {
		; Since other script threads can interrupt during LONG_OPERATION_UPDATE, it's important that
		; this command not refer to sArgDeref[] and sArgVar[] anytime after an interruption becomes
		; possible. This is because an interrupting thread usually changes the values to something
		; inappropriate for this thread.
		tick_now := A_TickCount
		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", PM_NOREMOVE)
				Sleep -1
			tick_now := A_TickCount
			g_script.mLastPeekTime := tick_now
		}
		if (current_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ; skip any matching directories.
			continue
		if (StrLen(StrGet(current_file.cFileName[""])) > space_remaining)
		{
			; v1.0.45.03: Don't even try to operate upon truncated filenames in case they accidentally
			; match the name of a real/existing file.
			g.LastError := ERROR_BUFFER_OVERFLOW
			++failure_count
		}
		else
		{
			StrPut(StrGet(append_pos), current_file.cFileName[""]) ; Above has ensured this won't overflow.
			if !DllCall("DeleteFile","Str",file_path)
			{
				g.LastError := DllCall("GetLastError")
				++failure_count
			}
		}
	} Until !DllCall("FindNextFile","PTR",file_search,"PTR", current_file[])
	DllCall("FindClose","PTR",file_search)
	ErrorLevel:=failure_count
	return  ; i.e. indicate success if there were no failures.
}