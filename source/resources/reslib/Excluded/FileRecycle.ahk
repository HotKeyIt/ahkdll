FileRecycle(aFilePattern){
  static FO_DELETE:=3,FOF_SILENT:=4,FOF_ALLOWUNDO:=64,FOF_NOCONFIRMATION:=16,FOF_WANTNUKEWARNING:=16384,MAX_PATH:=260,szFileTemp,init:=VarSetCapacity(szFileTemp,260*2,0)
        ,FileOp:=Struct("HWND hwnd;UINT wFunc;PCTSTR pFrom;PCTSTR pTo;WORD fFlags;BOOL fAnyOperationsAborted;LPVOID hNameMappings;PCTSTR lpszProgressTitle")
	if (aFilePattern=""){
		ErrorLevel := true ; Since this is probably not what the user intended.
    return 
  }
	; au3: Get the fullpathname - required for UNDO to worku
	DllCall("GetFullPathName","Str",aFilePattern,"Int", MAX_PATH,"PTR", &szFileTemp,"PTR", 0)
	VarSetCapacity(szFileTemp,-1)

	; au3: We must also make it a double nulled string *sigh*
  ; already done in VarSetCapacity
  ; au3: set to known values - Corrects crash
	FileOp.hNameMappings := 0
	FileOp.lpszProgressTitle := 0
	FileOp.fAnyOperationsAborted := FALSE
	FileOp.hwnd := 0
	FileOp.pTo := 0

	FileOp.pFrom := &szFileTemp
	FileOp.wFunc := FO_DELETE
	FileOp.fFlags := FOF_SILENT | FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_WANTNUKEWARNING
	; SHFileOperation() returns 0 on success:
  ErrorLevel := DllCall("shell32\SHFileOperationW","PTR",FileOp[])
}