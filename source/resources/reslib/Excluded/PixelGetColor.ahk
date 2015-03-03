PixelGetColor(aX,aY,aOptions:=""){
	static COORD_MODE_CLIENT := 0, COORD_MODE_WINDOW := 1,COORD_MODE_SCREEN := 2
        ,pt := Struct("x,y"),rc:= Struct("left,top,right,bottom")
	if (aOptions="Slow") ; New mode for v1.0.43.10.  Takes precedence over Alt mode.
		return PixelSearch(aX, aY, aX, aY, 0, 0, aOptions, true) ; It takes care of setting ErrorLevel and the output-var.
	pt.Fill(),ErrorLevel:=0 ;Set default ErrorLevel.
	if (A_CoordModePixel != COORD_MODE_SCREEN){
		active_window := DllCall("GetForegroundWindow","PTR")
		if (active_window && !DllCall("IsIconic","PTR",active_window)){
			if (A_CoordModePixel = COORD_MODE_WINDOW){
				if DllCall("GetWindowRect","PTR",active_window,"PTR", rc[""])
					aX += rc.left,aY += rc.top
			}	else if DllCall("ClientToScreen","PTR", active_window,"PTR", pt[""])  ; (coord_mode == COORD_MODE_CLIENT)
					aX += pt.x,aY += pt.y
		}
	}
	use_alt_mode:=InStr(aOptions,"Alt")
	If !(hdc:=(use_alt_mode ? DllCall("CreateDC","Str","DISPLAY","Str","","Str","","PTR",0,"PTR") : DllCall("GetDC","PTR",0,"PTR"))){
    ErrorLevel:=1
		return
  }

	;// Assign the value as an 32-bit int to match Window Spy reports color values.
	;// Update for v1.0.21: Assigning in hex format seems much better, since it's easy to
	;// look at a hex BGR value to get some idea of the hue.  In addition, the result
	;// is zero padded to make it easier to convert to RGB and more consistent in
	;// appearance:
	color := DllCall("GetPixel","PTR",hdc,"Int",aX,"Int",aY)
	if (use_alt_mode)
		DllCall("DeleteDC","PTR",hdc)
	else
		DllCall("ReleaseDC","PTR",0,"PTR",hdc)
	color:=format("0x{1:X}",InStr(aOptions,"RGB") ? ((Color&255)<<16) + (((Color>>8)&255)<<8) + (Color>>16) : (color))
	return color
}