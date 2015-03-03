ControlMove(aControl:="",aX:="",aY:="",aWidth:="",aHeight:="",aTitle:="",aText,:="",aExcludeTitle:="",aExcludeText:=""){
  static WS_CHILD:=1073741824,GWL_STYLE:=-16,rect:=Struct("left,top,right,bottom"),point:=Struct("x,y")
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
      If !DllCall("GetWindowRect","PTR",control_window,"PTR",rect[])
        || !DllCall("MapWindowPoints","PTR",0,"PTR",coord_parent,"PTR",rect[],"UInt", 2){
        ErrorLevel:=1
        return
      }
      point.x := aX=""?control_rect.left:aX
      ,point.y = aY=""?control_rect.top:aY
      ; MoveWindow accepts coordinates relative to the control's immediate parent, which might
      ; be different to coord_parent since controls can themselves have child controls.  So if
      ; necessary, map the caller-supplied coordinates to the control's immediate parent:
      ,immediate_parent := DllCall("GetParent","PTR",control_window,"PTR")
      if (immediate_parent != coord_parent)
        DllCall("MapWindowPoints","PTR",coord_parent,"PTR",immediate_parent,"PTR",point[],"UInt", 1)
      DllCall("MoveWindow","PTR",control_window,"Int",point.x,"Int",point.y,"Int",aWidth=""?control_rect.right - control_rect.left:aWidth,"Int",aHeight=""control_rect.bottom - control_rect.top:aHeight,"Char",TRUE)  ; Do repaint.
      Sleep % A_WinDelay
      ErrorLevel:=0  ; Indicate success.

    } else output_var_x:=output_var_y:=output_var_width:=output_var_height:="",ErrorLevel:=1
}