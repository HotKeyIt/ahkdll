ControlGetPos(ByRef output_var_x:="",ByRef output_var_y:="",ByRef output_var_width:="",ByRef output_var_height:="",aControl:="",aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
  static WS_CHILD:=1073741824,GWL_STYLE:=-16,rect:=Struct("left,top,right,bottom")
	if target_window := aTitle aText=""?WinExist():WinExist(aTitle, aText, aExcludeTitle, aExcludeText)
    if control_window := ControlGet("HWND","",aControl,"ahk_id " target_window){ ; This can return target_window itself for cases such as ahk_id %ControlHWND%.
      ; Determine which window the returned coordinates should be relative to:
      coord_parent:=control_window
      Loop {
        if !DllCall("GetWindowLong","PTR",coord_parent,"UInt",GWL_STYLE) & WS_CHILD  ; Found the first non-child parent, so return it.
          break
        ; Because Windows 95 doesn't support GetAncestor(), we'll use GetParent() instead:
        else if !parent := DllCall("GetParent","PTR",coord_parent)
          break  ; This will return aWnd if aWnd has no parents.
        else parent_prev:=parent
      }
      ; Realistically never fails since DetermineTargetWindow() and ControlExist() should always yield
      ; valid window handles:
      DllCall("GetWindowRect","PTR",control_window,"PTR",rect[])
      ; Map the screen coordinates returned by GetWindowRect to the client area of coord_parent.
      DllCall("MapWindowPoints","PTR",0,"PTR",coord_parent,"PTR",rect[],"UInt", 2)
      output_var_x:=rect.left,output_var_y:=rect.top,output_var_width:=rect.right-rect.left,output_var_height:=rect.bottom-rect.top
    } else output_var_x:=output_var_y:=output_var_width:=output_var_height:="",ErrorLevel:=1
}
