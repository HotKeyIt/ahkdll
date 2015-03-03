FileRecycleEmpty(aDriveLetter){
	; Not using GetModuleHandle() because there is doubt that SHELL32 (unlike USER32/KERNEL32), is
	; always automatically present in every process (e.g. if shell is something other than Explorer):

	; Get the address of all the functions we require
	;~ typedef HRESULT (WINAPI *MySHEmptyRecycleBin)(HWND, LPCTSTR, DWORD)
	if DllCall("shell32\SHEmptyRecycleBin","PTR", 0,"Str", aDriveLetter,"UInt"
						, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND)
		ErrorLevel := true
	else ErrorLevel := false
}
