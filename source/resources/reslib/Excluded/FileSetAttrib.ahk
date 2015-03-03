FileSetAttrib(aAttributes, aFilePattern, aOperateOnFolders:=0,aDoRecurse:=0,ByRef aCalledRecursively:=0){
; Returns the number of files and folders that could not be changed due to an error.
  static g:=GlobalStruct(),g_script:=ScriptStruct(),ERROR_INVALID_PARAMETER:=87,ERROR_BUFFER_OVERFLOW:=111
				,FILE_LOOP_INVALID:=0,FILE_LOOP_FILES_ONLY:=1,FILE_LOOP_FOLDERS_ONLY:=2,FILE_LOOP_RECURSE:=4
        ,MAX_PATH:=260,file_pattern:=Struct("SHORT[260]"), file_path:=Struct("SHORT[260]"),ATTRIB_MODE_NONE:=0, ATTRIB_MODE_ADD:=1, ATTRIB_MODE_REMOVE:=2, ATTRIB_MODE_TOGGLE:=3
				,FILETIME:="DWORD dwLowDateTime;DWORD dwHighDateTime"
        ,WIN32_FIND_DATA:="DWORD dwFileAttributes;FileSetAttrib(FILETIME) ftCreationTime;FileSetAttrib(FILETIME) ftLastAccessTime;FileSetAttrib(FILETIME) ftLastWriteTime;DWORD nFileSizeHigh;DWORD nFileSizeLow;DWORD dwReserved0;DWORD dwReserved1;TCHAR cFileName[260];TCHAR cAlternateFileName[14]"
				,current_file:=Struct(WIN32_FIND_DATA),FILE_ATTRIBUTE_DIRECTORY:=16,FILE_ATTRIBUTE_READONLY:=1,FILE_ATTRIBUTE_ARCHIVE:=32
				,FILE_ATTRIBUTE_SYSTEM:=4,FILE_ATTRIBUTE_HIDDEN:=2,FILE_ATTRIBUTE_NORMAL:=128,FILE_ATTRIBUTE_OFFLINE:=4096,FILE_ATTRIBUTE_TEMPORARY:=256
				
				
	failure_count:=0
	if !recurse:=aCalledRecursively{  ; i.e. Only need to do this if we're not called by ourself:
		if (aFilePattern=""){
			g.LastError := ERROR_INVALID_PARAMETER
			return
		}
		if (aOperateOnFolders = FILE_LOOP_INVALID) ; In case runtime dereference of a var was an invalid value.
			aOperateOnFolders := FILE_LOOP_FILES_ONLY  ; Set default.
		g.LastError := 0 ; Set default. Overridden only when a failure occurs.
	}
	aCalledRecursively:=0
	if (StrLen(aFilePattern) >= MAX_PATH) ; Checked early to simplify other things below.
	{
		g.LastError := ERROR_BUFFER_OVERFLOW
		return
	}

	; Related to the comment at the top: Since the script subroutine that resulted in the call to
	; this function can be interrupted during our MsgSleep(), make a copy of any params that might
	; currently point directly to the deref buffer.  This is done because their contents might
	; be overwritten by the interrupting subroutine:

	; Testing shows that the ANSI version of FindFirstFile() will not accept a path+pattern longer
	; than 256 or so, even if the pattern would match files whose names are short enough to be legal.
	; Therefore, as of v1.0.25, there is also a hard limit of MAX_PATH on all these variables.
	; MSDN confirms this in a vague way: "In the ANSI version of FindFirstFile(), [plpFileName] is
	; limited to MAX_PATH characters."
	 ; Giving +3 extra for "*.*" seems fairly pointless because any files that actually need that extra room would fail to be retrieved by FindFirst/Next due to their inability to support paths much over 256.
	StrPut(aFilePattern,file_pattern[]),StrPut(aFilePattern,file_path[])

	if last_backslash := InStr(StrGet(file_path[]), "\",1,-1) {
		; Remove the filename and/or wildcard part.   But leave the trailing backslash on it for
		; consistency with below:
    file_path[last_backslash+1]:=0
		file_path_length := last_backslash
	}
	else ; Use current working directory, e.g. if user specified only *.*
	{
		file_path.1 := 0
		file_path_length := 0
	}
	append_pos := file_path[] + file_path_length*2 ; For performance, copy in the unchanging part only once.  This is where the changing part gets appended.
	
	space_remaining := 260 - file_path_length - 1 ; Space left in file_path for the changing part.

	; For use with aDoRecurse, get just the naked file name/pattern:
	naked_filename_or_pattern := file_pattern[] + (last_backslash?(last_backslash+1)*2:0)

	if !InStr(StrGet(naked_filename_or_pattern), "?") && !InStr(StrGet(naked_filename_or_pattern),"*")
		; Since no wildcards, always operate on this single item even if it's a folder.
		aOperateOnFolders := FILE_LOOP_FILES_AND_FOLDERS

	mode := ATTRIB_MODE_NONE

	failure_count := 0

	if -1 != file_search := DllCall("FindFirstFile","PTR",file_pattern[],"PTR",current_file[],"PTR")
	{
		Loop
		{
			; Since other script threads can interrupt during LONG_OPERATION_UPDATE, it's important that
			; this command not refer to sArgDeref[] and sArgVar[] anytime after an interruption becomes
			; possible. This is because an interrupting thread usually changes the values to something
			; inappropriate for this thread.
			tick_now := A_TickCount
  		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
  			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", 0) ;PM_NOREMOVE
  				Sleep -1
  			tick_now := A_TickCount
  			g_script.mLastPeekTime := tick_now
  		}

			if (current_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (current_file.cFileName[1] == "." && (!current_file.cFileName[2]    ; Relies on short-circuit boolean order.
					|| current_file.cFileName[2] == "." && !current_file.cFileName[3]) ;
					; Regardless of whether this folder will be recursed into, this folder's own attributes
					; will not be affected when the mode is files-only:
					|| aOperateOnFolders == FILE_LOOP_FILES_ONLY)
					continue ; Never operate upon or recurse into these.
			}
			else ; It's a file, not a folder.
				if (aOperateOnFolders == FILE_LOOP_FOLDERS_ONLY)
					continue
			
			if (StrLen(StrGet(current_file.cFileName[""])) > space_remaining)
			{
				; v1.0.45.03: Don't even try to operate upon truncated filenames in case they accidentally
				; match the name of a real/existing file.
				g.LastError := ERROR_BUFFER_OVERFLOW
				++failure_count
				continue
			}
			; Otherwise, make file_path be the filespec of the file to operate upon:
			Strput(StrGet(current_file.cFileName[""]),append_pos) ; Above has ensured this won't overflow.

			LoopParse,%aAttributes%
			{
				if A_LoopField="+"
          mode:=ATTRIB_MODE_ADD
        else if A_LoopField="-"
          mode:=ATTRIB_MODE_REMOVE
        else if A_LoopField="^"
          mode:=ATTRIB_MODE_TOGGLE
				; Note that D (directory) and C (compressed) are currently not supported:
				else if mode = ATTRIB_MODE_ADD {
          if A_LoopField="R"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_READONLY
          else if A_LoopField="A"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE
          else if A_LoopField="S"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM
          else if A_LoopField="H"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN
          else if A_LoopField="N"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL
          else if A_LoopField="O"
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_OFFLINE
          else if A_LoopField="T" 
            current_file.dwFileAttributes |= FILE_ATTRIBUTE_TEMPORARY
				}else if mode == ATTRIB_MODE_REMOVE {
            if A_LoopField="R"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY
          else if A_LoopField="A"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE
          else if A_LoopField="S"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_SYSTEM
          else if A_LoopField="H"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN
          else if A_LoopField="N"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_NORMAL
          else if A_LoopField="O"
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE
          else if A_LoopField="T" 
            current_file.dwFileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY
				}else if mode == ATTRIB_MODE_TOGGLE {
					if A_LoopField="R"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_READONLY
          else if A_LoopField="A"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_ARCHIVE
          else if A_LoopField="S"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_SYSTEM
          else if A_LoopField="H"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_HIDDEN
          else if A_LoopField="N"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_NORMAL
          else if A_LoopField="O"
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_OFFLINE
          else if A_LoopField="T" 
            current_file.dwFileAttributes ^= FILE_ATTRIBUTE_TEMPORARY
				}
			}
			if (!DllCall("SetFileAttributes","PTR",file_path[],"UInt",current_file.dwFileAttributes))
				++failure_count
		} Until (!DllCall("FindNextFile","PTR",file_search,"PTR",current_file[]))

		DllCall("FindClose","PTR",file_search)
	} ; if (file_search != INVALID_HANDLE_VALUE)

	if (aDoRecurse && space_remaining > 2) ; The space_remaining check ensures there's enough room to append "*.*" (if not, just avoid recursing into it due to rarity).
	{
		; Testing shows that the ANSI version of FindFirstFile() will not accept a path+pattern longer
		; than 256 or so, even if the pattern would match files whose names are short enough to be legal.
		; Therefore, as of v1.0.25, there is also a hard limit of MAX_PATH on all these variables.
		; MSDN confirms this in a vague way: "In the ANSI version of FindFirstFile(), [plpFileName] is
		; limited to MAX_PATH characters."
		StrPut("*.*",append_pos) ; Above has ensured this won't overflow.
		if -1 != file_search := DllCall("FindFirstFile","PTR",file_path[],"PTR",current_file[]){
			pattern_length := StrLen(StrGet(naked_filename_or_pattern[]))
			Loop
			{
				tick_now := A_TickCount
    		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
    			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", 0) ;PM_NOREMOVE
    				Sleep -1
    			tick_now := A_TickCount
    			g_script.mLastPeekTime := tick_now
    		}
				if (!(current_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					|| current_file.cFileName[1] == "." && (!current_file.cFileName[2]     ; Relies on short-circuit boolean order.
						|| current_file.cFileName[2] == "." && !current_file.cFileName[3]) ;
					; v1.0.45.03: Skip over folders whose full-path-names are too long to be supported by the ANSI
					; versions of FindFirst/FindNext.  Without this fix, it might be possible for infinite recursion
					; to occur (see PerformLoop() for more comments).
					|| pattern_length + StrLen(StrGet(current_file.cFileName[""])) >= space_remaining) ; >= vs. > to reserve 1 for the backslash to be added between cFileName and naked_filename_or_pattern.
					continue ; Never recurse into these.
				; This will build the string CurrentDir+SubDir+FilePatternOrName.
				; If FilePatternOrName doesn't contain a wildcard, the recursion
				; process will attempt to operate on the originally-specified
				; single filename or folder name if it occurs anywhere else in the
				; tree, e.g. recursing C:\Temp\temp.txt would affect all occurrences
				; of temp.txt both in C:\Temp and any subdirectories it might contain:
				StrPut(StrGet(current_file.cFileName[""]) "\" StrGet(naked_filename_or_pattern),append_pos)
				FileSetAttrib(aAttributes,StrGet(file_path[]),aOperateOnFolders,aDoRecurse,recurse_failure:=1)
        failure_count += recurse_failure
			} Until !DllCall("FindNextFile","PTR",file_search,"PTR",current_file[])
			DllCall("FindClose","PTR",file_search)
		} ; if (file_search != INVALID_HANDLE_VALUE)
	} ; if (aDoRecurse)

	if (!recurse) ; i.e. Only need to do this if we're returning to top-level caller:
		ErrorLevel:=failure_count ; i.e. indicate success if there were no failures.
  else aCalledRecursively:=failure_count
}