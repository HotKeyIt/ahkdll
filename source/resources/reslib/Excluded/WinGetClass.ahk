WinGetClass(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
  static WINDOW_CLASS_SIZE:=257,class_name,init:=VarSetCapacity(class_name,WINDOW_CLASS_SIZE,0)
	; WinGetClass causes an access violation if one of the script's windows is sub-classed by the script [unless the above is done].
	; This occurs because WM_GETTEXT is sent to the GUI, triggering the window procedure. The script callback
	; then executes and invalidates sArgVar[0], which WinGetClass attempts to dereference. 
	; (Thanks to TodWulff for bringing this issue to my attention.) 
	; Solution: WinGetTitle resolves the OUTPUT_VAR (*sArgVar) macro once, before searching for the window.
	; I suggest the same be done for WinGetClass.
	if !(target_window := aTitle aText=""?WinExist():WinExist(aTitle, aText, aExcludeTitle, aExcludeText))
		|| !DllCall("GetClassName","PTR",target_window,"Str", class_name,"UInt",WINDOW_CLASS_SIZE)
		return
	return VarSetCapacity(class_name,-1),class_name
}
