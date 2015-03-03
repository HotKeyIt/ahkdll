WinGetTitle(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
	target_window := aTitle aText=""?WinExist():WinExist(aTitle, aText, aExcludeTitle, aExcludeText)
	; Even if target_window is NULL, we want to continue on so that the output
	; param is set to be the empty string, which is the proper thing to do
	; rather than leaving whatever was in there before.

	; Handle the output parameter.  See the comments in ACT_CONTROLGETTEXT for details.
	space_needed := target_window?DllCall("GetWindowTextLength","PTR",target_window)+1:1 ; 1 for terminator.
	if (target_window){
    VarSetCapacity(Title,space_needed,0)
    ErrorLevel:=!DllCall("GetWindowText","PTR",target_window,"Str",Title,"UInt",space_needed)
	}
	return Title
}
