WinGetPos(ByRef output_var_x:="",ByRef output_var_y:="", ByRef output_var_width:="",ByRef output_var_height:="",aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
	static rect:=Struct("left,top,right,bottom")
	; Even if target_window is NULL, we want to continue on so that the output
	; variables are set to be the empty string, which is the proper thing to do
	; rather than leaving whatever was in there before.
	rect.Fill()
	if target_window := aTitle aText=""?WinExist():WinExist(aTitle, aText, aExcludeTitle, aExcludeText)
		DllCall("GetWindowRect","PTR",target_window,"PTR",rect[])
    ,output_var_x:=rect.left,output_var_y:=rect.top,ErrorLevel:=0
    ,output_var_width:=rect.right,output_var_height:=rect.bottom
  else ErrorLevel:=1,output_var_x:="",output_var_y:="",output_var_width:="",output_var_height:=""
}
