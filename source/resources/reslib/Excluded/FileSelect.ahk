FileSelect(aOptions:="", aWorkingDir:="", aTitle:="", aFilter:=""){
; Since other script threads can interrupt this command while it's running, it's important that
; this command not refer to sArgDeref[] and sArgVar[] anytime after an interruption becomes possible.
; This is because an interrupting thread usually changes the values to something inappropriate for this thread.
	static OPENFILENAME:="DWORD lStructSize;HWND hwndOwner;HINSTANCE hInstance;LPCTSTR lpstrFilter;LPTSTR lpstrCustomFilter;DWORD nMaxCustFilter;DWORD nFilterIndex;LPTSTR lpstrFile;DWORD nMaxFile;LPTSTR lpstrFileTitle;DWORD nMaxFileTitle;LPCTSTR lpstrInitialDir;LPCTSTR lpstrTitle;DWORD Flags;WORD nFileOffset;WORD nFileExtension;LPCTSTR lpstrDefExt;LPARAM lCustData;PTR lpfnHook;LPCTSTR lpTemplateName;void *pvReserved;DWORD dwReserved;DWORD FlagsEx"
        ,FILE_ATTRIBUTE_DIRECTORY:=16,ofn:=Struct(OPENFILENAME)
				,OFN_HIDEREADONLY:=4,OFN_EXPLORER:=524288  ; OFN_HIDEREADONLY: Hides the Read Only check box.
				,OFN_NODEREFERENCELINKS:=1048576,OFN_OVERWRITEPROMPT:=2,OFN_CREATEPROMPT:=8192,OFN_ALLOWMULTISELECT:=512
				,OFN_PATHMUSTEXIST:=2048,OFN_FILEMUSTEXIST:=4096
	;~ Var &output_var = *OUTPUT_VAR ; Fix for v1.0.45.01: Must be resolved and saved early.  See comment above.
	;~ if (g_nFileDialogs >= MAX_FILEDIALOGS)
	;~ {
		;~ ; Have a maximum to help prevent runaway hotkeys due to key-repeat feature, etc.
		;~ return LineError(_T("The maximum number of File Dialogs has been reached."))
	;~ }
	
	; Large in case more than one file is allowed to be selected.
	; The call to GetOpenFileName() may fail if the first character of the buffer isn't NULL
	; because it then thinks the buffer contains the default filename, which if it's uninitialized
	; may be a string that's too long.
	;~ TCHAR file_buf[65535] = _T("") ; Set default.
	VarSetCapacity(file_buf,65535,0)
	g_WorkingDir:=A_WorkingDir
	;~ TCHAR working_dir[MAX_PATH]
	;~ if (!aWorkingDir || !*aWorkingDir)
		;~ *working_dir = '\0'
	;~ else
	If (working_dir:=aWorkingDir){
		;~ tcslcpy(working_dir, aWorkingDir, _countof(working_dir))
		; v1.0.43.10: Support CLSIDs such as:
		;   My Computer  ::{20d04fe0-3aea-1069-a2d8-08002b30309d}
		;   My Documents ::{450d8fba-ad25-11d0-98a8-0800361b1103}
		; Also support optional subdirectory appended to the CLSID.
		; Neither SetCurrentDirectory() nor GetFileAttributes() directly supports CLSIDs, so rely on other means
		; to detect whether a CLSID ends in a directory vs. filename.
		;~ bool is_directory, is_clsid
		if (is_clsid := InStr(working_dir, "::{")){
			;~ LPTSTR end_brace
			if (end_brace := InStr(working_dir, "}"))
				is_directory := StrLen(working_dir)>end_brace ; First '}' is also the last char in string, so it's naked CLSID (so assume directory).
					|| SubStr(working_dir,-1) = "\" ; Or path ends in backslash.
			else ; Badly formatted clsid.
				is_directory := true ; Arbitrary default due to rarity.
		}
		else ; Not a CLSID.
		{
			attr := DllCall("GetFileAttributes","Str",working_dir)
			is_directory := (attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY)
		}
		if (!is_directory)
		{
			; Above condition indicates it's either an existing file that's not a folder, or a nonexistent
			; folder/filename.  In either case, it seems best to assume it's a file because the user may want
			; to provide a default SAVE filename, and it would be normal for such a file not to already exist.
			if (last_backslash := InStr(working_dir, "\",1,-1))
			{
				file_buf:=SubStr(working_dir, last_backslash + 1) ; Set the default filename.
				working_dir:=SubStr(working_dir,1,last_backslash-1) ; Make the working directory just the file's path.
			}
			else ; The entire working_dir string is the default file (unless this is a clsid).
				if (!is_clsid)
				{
					file_buf:= working_dir
					;~ *working_dir = '\0'  ; This signals it to use the default directory.
				}
				;else leave working_dir set to the entire clsid string in case it's somehow valid.
		}
		; else it is a directory, so just leave working_dir set as it was initially.
	}

	greeting:=aTitle?aTitle:"Select File - " A_ScriptName
	;~ if (aTitle && *aTitle)
		;~ tcslcpy(greeting, aTitle, _countof(greeting))
	;~ else
		;~ ; Use a more specific title so that the dialogs of different scripts can be distinguished
		;~ ; from one another, which may help script automation in rare cases:
		;~ sntprintf(greeting, _countof(greeting), _T("Select File - %s"), g_script.mFileName)

	; The filter must be terminated by two NULL characters.  One is explicit, the other automatic:
	;~ TCHAR filter[1024] = _T(""), pattern[1024] = _T("")  ; Set default.
	VarSetCapacity(filter,1024,0)
	if (aFilter){
		if (pattern_start := InStr(aFilter, "(")){
			; Make pattern a separate string because we want to remove any spaces from it.
			; For example, if the user specified Documents (*.txt *.doc), the space after
			; the semicolon should be removed for the pattern string itself but not from
			; the displayed version of the pattern:
			;~ tcslcpy(pattern, ++pattern_start, _countof(pattern))
			pattern:=SubStr(aFilter,++pattern_start)
			if (pattern_end := InStr(pattern, ")",1,-1)) ; strrchr() in case there are other literal parentheses.
				pattern := SubStr(pattern,1,pattern_end-1)  ; If parentheses are empty, this will set pattern to be the empty string.
			else ; no closing paren, so set to empty string as an indicator:
				pattern := ""

		}
		else ; No open-paren, so assume the entire string is the filter.
			pattern := aFilter
		if (pattern){
			; Remove any spaces present in the pattern, such as a space after every semicolon
			; that separates the allowed file extensions.  The API docs specify that there
			; should be no spaces in the pattern itself, even though it's okay if they exist
			; in the displayed name of the file-type:
			StrReplace,pattern,% pattern,%A_Space%
			; Also include the All Files (*.*) filter, since there doesn't seem to be much
			; point to making this an option.  This is because the user could always type
			; *.* and press ENTER in the filename field and achieve the same result:
			StrPut(aFilter,&filter)
			StrPut(pattern,(&filter) + StrLen(aFilter)*2 + 2)
			StrPut("All Files (*.*)",(&filter) + (StrLen(aFilter)+StrLen(pattern))*2 + 4)
			StrPut("*.*",(&filter) + (StrLen(aFilter)+StrLen(pattern))*2 + 6 + 30)
			;~ sntprintf(filter, _countof(filter), _T("%s%c%s%cAll Files (*.*)%c*.*%c")
				;~ , aFilter, '\0', pattern, '\0', '\0', '\0') ; The final '\0' double-terminates by virtue of the fact that sntprintf() itself provides a final terminator.
		}
		else
			StrPut("",&filter)  ; It will use a standard default below.
	}

	ofn.Fill()
	; OPENFILENAME_SIZE_VERSION_400 must be used for 9x/NT otherwise the dialog will not appear!
	; MSDN: "In an application that is compiled with WINVER and _WIN32_WINNT >= 0x0500, use
	; OPENFILENAME_SIZE_VERSION_400 for this member.  Windows 2000/XP: Use sizeof(OPENFILENAME)
	; for this parameter."
	ofn.lStructSize := sizeof(ofn)
	ofn.hwndOwner := 0 ;THREAD_DIALOG_OWNER ; Can be NULL, which is used instead of main window since no need to have main window forced into the background for this.
	ofn.lpstrTitle[""] := &greeting
	If !filter {
		StrPut("All Files (*.*)",&filter)
		StrPut("*.*",(&filter) + 32)
		StrPut("Text Documents (*.txt)",(&filter) + 40)
		StrPut("*.txt",(&filter) + 86)
	}
	ofn.lpstrFilter[""] := &filter ;? filter : _T("All Files (*.*)\0*.*\0Text Documents (*.txt)\0*.txt\0")
	ofn.lpstrFile[""] := &file_buf
	ofn.nMaxFile := VarSetCapacity(file_buf) - 1 ; -1 to be extra safe.
	; Specifying NULL will make it default to the last used directory (at least in Win2k):
	ofn.lpstrInitialDir[""] := working_dir ? &working_dir : 0

	; Note that the OFN_NOCHANGEDIR flag is ineffective in some cases, so we'll use a custom
	; workaround instead.  MSDN: "Windows NT 4.0/2000/XP: This flag is ineffective for GetOpenFileName."
	; In addition, it does not prevent the CWD from changing while the user navigates from folder to
	; folder in the dialog, except perhaps on Win9x.

	; For v1.0.25.05, the new "M" letter is used for a new multi-select method since the old multi-select
	; is faulty in the following ways:
	; 1) If the user selects a single file in a multi-select dialog, the result is inconsistent: it
	;    contains the full path and name of that single file rather than the folder followed by the
	;    single file name as most users would expect.  To make matters worse, it includes a linefeed
	;    after that full path in name, which makes it difficult for a script to determine whether
	;    only a single file was selected.
	; 2) The last item in the list is terminated by a linefeed, which is not as easily used with a
	;    parsing loop as shown in example in the help file.
	always_use_save_dialog := false ; Set default.
	new_multi_select_method := false ; Set default.
	;~ switch (ctoupper(*aOptions))
	;~ {
	If (InStr(aOptions,"M")=1){  ; Multi-select.
		aOptions:=SubStr(aOptions,2)
		new_multi_select_method := true
	} else if (InStr(aOptions,"S")=1){ ; Have a "Save" button rather than an "Open" button.
		aOptions:=SubStr(aOptions,2)
		always_use_save_dialog := true
	}

	options := aOptions
	; v1.0.43.09: OFN_NODEREFERENCELINKS is now omitted by default because most people probably want a click
	; on a shortcut to navigate to the shortcut's target rather than select the shortcut and end the dialog.
	ofn.Flags := OFN_HIDEREADONLY | OFN_EXPLORER  ; OFN_HIDEREADONLY: Hides the Read Only check box.
	if (options & 0x20) ; v1.0.43.09.
		ofn.Flags |= OFN_NODEREFERENCELINKS
	if (options & 0x10)
		ofn.Flags |= OFN_OVERWRITEPROMPT
	if (options & 0x08)
		ofn.Flags |= OFN_CREATEPROMPT
	if (new_multi_select_method)
		ofn.Flags |= OFN_ALLOWMULTISELECT
	if (options & 0x02)
		ofn.Flags |= OFN_PATHMUSTEXIST
	if (options & 0x01)
		ofn.Flags |= OFN_FILEMUSTEXIST

	; At this point, we know a dialog will be displayed.  See macro's comments for details:
	;~ DIALOG_PREP
	;~ POST_AHK_DIALOG(0) ; Do this only after the above. Must pass 0 for timeout in this case.

	;~ ++g_nFileDialogs
	; Below: OFN_CREATEPROMPT doesn't seem to work with GetSaveFileName(), so always
	; use GetOpenFileName() in that case:
	result := (always_use_save_dialog || ((ofn.Flags & OFN_OVERWRITEPROMPT) && !(ofn.Flags & OFN_CREATEPROMPT)))
		? DllCall("Comdlg32\GetSaveFileName","PTR",ofn[]) : DllCall("Comdlg32\GetOpenFileName","PTR",ofn[])
	;~ --g_nFileDialogs
	;~ DIALOG_END

	; Both GetOpenFileName() and GetSaveFileName() change the working directory as a side-effect
	; of their operation.  The below is not a 100% workaround for the problem because even while
	; a new quasi-thread is running (having interrupted this one while the dialog is still
	; displayed), the dialog is still functional, and as a result, the dialog changes the
	; working directory every time the user navigates to a new folder.
	; This is only needed when the user pressed OK, since the dialog auto-restores the
	; working directory if CANCEL is pressed or the window was closed by means other than OK.
	; UPDATE: No, it's needed for CANCEL too because GetSaveFileName/GetOpenFileName will restore
	; the working dir to the wrong dir if the user changed it (via SetWorkingDir) while the
	; dialog was displayed.
	; Restore the original working directory so that any threads suspended beneath this one,
	; and any newly launched ones if there aren't any suspended threads, will have the directory
	; that the user expects.  NOTE: It's possible for g_WorkingDir to have changed via the
	; SetWorkingDir command while the dialog was displayed (e.g. a newly launched quasi-thread):
	SetWorkingDir,%g_WorkingDir%
	;~ if (*g_WorkingDir)
		;~ SetCurrentDirectory(g_WorkingDir)

	if (!result){ ; User pressed CANCEL vs. OK to dismiss the dialog or there was a problem displaying it.
		; It seems best to clear the variable in these cases, since this is a scripting
		; language where performance is not the primary goal.  So do that and return OK,
		; but leave ErrorLevel set to ERRORLEVEL_ERROR.
		return DllCall("Comdlg32\CommDlgExtendedError") ? -1 ; An error occurred.
																					 : 0 ; User pressed CANCEL, so never throw an exception.
	}
	;~ else
		;~ g_ErrorLevel->Assign(ERRORLEVEL_NONE) ; Indicate that the user pressed OK vs. CANCEL.

	if (ofn.Flags & OFN_ALLOWMULTISELECT){
		current_file:=StrGet(&file_buf)
		end_pointer:=(&file_buf) + StrLen(current_file)*2 + 2
		If !StrGet(end_pointer) ; only one file selected
			return current_file
		else {
			Loop {
				NumPut(13,end_pointer-2,"Short")
				end_pointer+=StrLen(StrGet(end_pointer))*2+2
			} Until (!NumGet(end_pointer,"Short"))
		}
	}
	return StrGet(&file_buf)
}
