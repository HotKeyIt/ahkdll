DirSelect(aRootDir:="", aOptions:="", aTitle:=""){
; Since other script threads can interrupt this command while it's running, it's important that
; the command not refer to sArgDeref[] and sArgVar[] anytime after an interruption becomes possible.
; This is because an interrupting thread usually changes the values to something inappropriate for this thread.
  static BROWSEINFO:="HWND hwndOwner;Int pidlRoot;LPTSTR pszDisplayName;LPCTSTR lpszTitle;UINT ulFlags;PTR lpfn;LPARAM lParam;int iImage"
        ,bi:=Struct("FileSelectFolder(BROWSEINFO)"),FileSelectFolderCallback:=RegisterCallback("FileSelectFolderCallback","",4),MAX_PATH:=260
        ,FSF_ALLOW_CREATE:=1,FSF_NONEWDIALOG:=4,BIF_NEWDIALOGSTYLE:=0x40,BIF_NONEWFOLDERBUTTON:=0x200,FSF_EDITBOX:=2,BIF_EDITBOX:=0x10
        ;~ ,pDF,init1:=VarSetCapacity(pDF,16,0) DllCall("ole32\CLSIDFromString","Str", "{000214E6-0000-0000-C000-000000000046}", "PTR", &pDF)
        ;~ ,pMalloc,init2:=VarSetCapacity(pMalloc,16,0) DllCall("ole32\CLSIDFromString","Str", "{00000002-0000-0000-C000-000000000046}", "PTR", &pMalloc)
        ,root_dir,init:=VarSetCapacity(root_dir,(MAX_PATH*2 + 5)*2)
	;~ Var &output_var = *OUTPUT_VAR ; Must be resolved early.  See comment above.
	;~ if (!output_var.Assign())  ; Initialize the output variable.
		;~ return FAIL

	;~ if (g_nFolderDialogs >= MAX_FOLDERDIALOGS)
	;~ {
		;~ ; Have a maximum to help prevent runaway hotkeys due to key-repeat feature, etc.
		;~ return LineError(_T("The maximum number of Folder Dialogs has been reached."))
	;~ }
  ;~ VarSetCapacity(pMalloc,A_PtrSize)

  if DllCall("Shell32\SHGetMalloc","PTR*",pMalloc){	; Initialize
		ErrorLevel:=1
    return ;SetErrorLevelOrThrow()
  }
  ; v1.0.36.03: Support initial folder, which is different than the root folder because the root only
	; controls the origin point (above which the control cannot navigate).
	; Up to two paths might be present inside, including an asterisk and spaces between them.
	StrPut(aRootDir,&root_dir , (MAX_PATH*2 + 5) * 2) ; Make a modifiable copy.
  VarSetCapacity(root_dir,-1)
	if (initial_folder := InStr(root_dir, "*"))
	{
    NumPut(0,root_dir,initial_folder*2,"Short") ; Terminate so that root_dir becomes an isolated string.
		; Must eliminate the trailing whitespace or it won't work.  However, only up to one space or tab
		; so that path names that really do end in literal spaces can be used:
		if (initial_folder && (SubStr(root_dir,initial_folder-1)=" " || SubStr(root_dir,initial_folder-1) = "`t"))
			NumPut(0,root_dir,initial_folder*2-2,"Short") 
		; In case absolute paths can ever have literal leading whitespace, preserve that whitespace
		; by incrementing by only one and not calling omit_leading_whitespace().  This has been documented.
		initial_folder++
	}
	else
		initial_folder := 0
	if (ltrim(root_dir)="") ; Count all-whitespace as a blank string, but retain leading whitespace if there is also non-whitespace inside.
		NumPut(0,root_dir,"Short")

	if (initial_folder)
	{
		bi.lpfn := FileSelectFolderCallback
		bi.lParam := (&root_dir) + initial_folder*2  ; Used by the callback above.
	}
	else
		bi.lpfn := 0  ; It will ignore the value of bi.lParam when lpfn is NULL.
	if StrGet(&root_dir,1){
		if !DllCall("Shell32\SHGetDesktopFolder","PTR*",pDF){
      ;ParseDisplayName
			DllCall(NumGet(NumGet(pDF)+A_PtrSize*3),"PTR",pDF,"PTR",0,"PTR", 0,"Str", root_dir,"Int*", chEaten,"PTR*", pIdl,"Int*", dwAttributes)
			;Release
      DllCall(NumGet(NumGet(pDF)+A_PtrSize*2),"PTR",pDF)
			bi.pidlRoot := pIdl
		}
	}
	else ; No root directory.
		bi.pidlRoot := 0  ; Make it use "My Computer" as the root dir.

	bi.iImage := 0
	bi.hwndOwner := 0 ; THREAD_DIALOG_OWNER ; Can be NULL, which is used rather than main window since no need to have main window forced into the background by this.
	VarSetCapacity(greeting,1024)
	if (aTitle!="")
		StrPut(aTitle,&greeting,1024)
	else
		StrPut("Select Folder - " A_ScriptName,&greeting, 1024)
	bi.lpszTitle[""] := &greeting

	options := aOptions!="" ? aOptions : FSF_ALLOW_CREATE
	bi.ulFlags := ((options & FSF_NONEWDIALOG)    ? 0           : BIF_NEWDIALOGSTYLE) ; v1.0.48: Added to support BartPE/WinPE.
              | ((options & FSF_ALLOW_CREATE)   ? 0           : BIF_NONEWFOLDERBUTTON)
              | ((options & FSF_EDITBOX)        ? BIF_EDITBOX : 0)

	VarSetCapacity(Result,2048,0)
	bi.pszDisplayName[""] := &Result  ; This will hold the user's choice.

	; At this point, we know a dialog will be displayed.  See macro's comments for details:
	;~ DIALOG_PREP
	;~ POST_AHK_DIALOG(0) ; Do this only after the above.  Must pass 0 for timeout in this case.

	;~ ++g_nFolderDialogs
	lpItemIDList := DllCall("Shell32\SHBrowseForFolderW","PTR",bi[],"PTR")  ; Spawn Dialog
	;~ --g_nFolderDialogs

	;~ DIALOG_END
	if (!lpItemIDList){
		; Due to rarity and because there doesn't seem to be any way to detect it,
		; no exception is thrown when the function fails.  Instead, we just assume
		; that the user pressed CANCEL (which should not be treated as an error):
    ErrorLevel:=1
		return 
  }

	NumPut(0,&Result,"Short")  ; Reuse this var, this time to old the result of the below:
	DllCall("Shell32\SHGetPathFromIDListW","PTR",lpItemIDList,"PTR", &Result)
	DllCall(NumGet(NumGet(pMalloc,"PTR")+A_PtrSize*5),"PTR",pMalloc,"PTR",lpItemIDList) ;Free
	DllCall(NumGet(NumGet(pMalloc,"PTR")+A_PtrSize*2),"PTR",pMalloc) ;release
  ErrorLevel:=0
  VarSetCapacity(Result,-1)
	return Result
}
FileSelectFolderCallback(hwnd, uMsg, lParam, lpData){
  static BFFM_INITIALIZED:=1,BFFM_SETSELECTION:=1127,BFFM_VALIDATEFAILED:=4
	if (uMsg == BFFM_INITIALIZED) ; Caller has ensured that lpData isn't NULL by having set a valid lParam value.
		DllCall("SendMessage","PTR",hwnd,"Int", BFFM_SETSELECTION,"PTR", TRUE,"PTR", lpData)
	; In spite of the quote below, the behavior does not seem to vary regardless of what value is returned
	; upon receipt of BFFM_VALIDATEFAILED, at least on XP.  But in case it matters on other OSes, preserve
	; compatibility with versions older than 1.0.36.03 by keeping the dialog displayed even if the user enters
	; an invalid folder:
	; MSDN: "Returns zero except in the case of BFFM_VALIDATEFAILED. For that flag, returns zero to dismiss
	; the dialog or nonzero to keep the dialog displayed."
	return uMsg = BFFM_VALIDATEFAILED ; i.e. zero should be returned in almost every case.
}