FileGetSize(aFilespec, aGranularity:=""){
	static ULLONG_MAX:=0xffffffffffffffff,FILETIME:="DWORD dwLowDateTime;DWORD dwHighDateTime",MAX_PATH:=260
				,WIN32_FIND_DATA:="DWORD  dwFileAttributes;FileGetSize(FILETIME) ftCreationTime;FileGetSize(FILETIME) ftLastAccessTime;FileGetSize(FILETIME) ftLastWriteTime;DWORD nFileSizeHigh;DWORD nFileSizeLow;DWORD dwReserved0;DWORD dwReserved1;TCHAR cFileName[" MAX_PATH "];TCHAR cAlternateFileName[14]"
				,found_file:=Struct(WIN32_FIND_DATA)

	; Don't use CreateFile() & FileGetSize() because they will fail to work on a file that's in use.
	; Research indicates that this method has no disadvantages compared to the other method.
	If -1 = file_search := DllCall("FindFirstFile","Str",aFilespec=""?A_LoopFileFullPath:aFilespec,"PTR", found_file[],"PTR"){
		ErrorLevel:=1
		return ;SetErrorsOrThrow(true) ; Let ErrorLevel tell the story.
	}
	DllCall("FindClose","PTR",file_search)

	size := found_file.nFileSizeHigh << 32 | found_file.nFileSizeLow
	
	if (aGranularity="K") ; KB
		size //= 1024
	else if (aGranularity="M") ;MB
		size //= (1024 * 1024)
	; default: ; i.e. either 'B' for bytes, or blank, or some other unknown value, so default to bytes.
		; do nothing

	ErrorLevel := 0 ; Indicate success.
	return size > ULLONG_MAX ? -1 : size ; i.e. don't allow it to wrap around.
	; The below comment is obsolete in light of the switch to 64-bit integers.  But it might
	; be good to keep for background:
	; Currently, the above is basically subject to a 2 gig limit, I believe, after which the
	; size will appear to be negative.  Beyond a 4 gig limit, the value will probably wrap around
	; to zero and start counting from there as file sizes grow beyond 4 gig (UPDATE: The size
	; is now set to -1 [the maximum DWORD when expressed as a signed int] whenever >4 gig).
	; There's not much sense in putting values larger than 2 gig into the var as a text string
	; containing a positive number because such a var could never be properly handled by anything
	; that compares to it (e.g. IfGreater) or does math on it (e.g. EnvAdd), since those operations
	; use ATOI() to convert the string.  So as a future enhancement (unless the whole program is
	; revamped to use 64bit ints or something) might add an optional param to the end to indicate
	; size should be returned in K(ilobyte) or M(egabyte).  However, this is sorta bad too since
	; adding a param can break existing scripts which use filenames containing commas (delimiters)
	; with this command.  Therefore, I think I'll just add the K/M param now.
	; Also, the above assigns an int because unsigned ints should never be stored in script
	; variables.  This is because an unsigned variable larger than INT_MAX would not be properly
	; converted by ATOI(), which is current standard method for variables to be auto-converted
	; from text back to a number whenever that is needed.
}