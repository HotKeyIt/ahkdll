PixelSearch(ByRef x,ByRef y,aLeft, aTop, aRight, aBottom, aColorRGB, aVariation:=0, aOptions:="", aIsPixelGetColor:=0){
	; Author: The fast-mode PixelSearch was created by Aurelian Maga.
	; For maintainability, get options and RGB/BGR conversion out of the way early.
	static SRCCOPY:=0x00CC0020,POINT:="x,y",origin := Struct(POINT)
				 ,FillBitAnd:=MCodeH(A_PtrSize=8?"44894424184889542410894C24084883EC18C7042400000000EB088B0424FFC08904248B442420390424732048630424488B4C24288B5424308B048123C248630C24488B54242889048AEBCF4883C418C3":"558BEC83EC44535657C745FC00000000EB098B45FC83C0018945FC8B45FC3B450873178B45FC8B4D0C8B14812355108B45FC8B4D0C891481EBD85F5E5B8BE55DC3","i=uitui")
				/* FillBitAnd DEFINED ABOVE AS	FillBitAnd:=MCodeH(...)
					void FillBitAnd(unsigned int count,unsigned int pixel[],unsigned int value)
					{
						for(int i = 0;i<count;i++)
							pixel[i] &= value;
					}
				*/
        ,COORD_MODE_CLIENT := 0, COORD_MODE_WINDOW := 1,COORD_MODE_SCREEN := 2
        ,rc:= Struct("left,top,right,bottom"),pt:=Struct("x,y")
	
	fast_mode := aIsPixelGetColor || InStr(aOptions, "Fast")
	,aColorBGR := ((aColorRGB&255)<<16) + (((aColorRGB>>8)&255)<<8) + (aColorRGB>>16) ; RGB_TO_BGR

	; Many of the following sections are similar to those in ImageSearch(), so they should be
	; maintained together.
	,pt:=Struct(POINT)
	
	,origin.Fill(),pt.Fill()
	if (A_CoordModePixel != COORD_MODE_SCREEN){
		active_window := DllCall("GetForegroundWindow","PTR")
		if (active_window && !DllCall("IsIconic","PTR",active_window)){
			if (A_CoordModePixel = COORD_MODE_WINDOW){
				if DllCall("GetWindowRect","PTR",active_window,"PTR", rc[""])
					origin.x += rc.left,origin.y += rc.top
			}	else if DllCall("ClientToScreen","PTR", active_window,"PTR", pt[""])  ; (coord_mode == COORD_MODE_CLIENT)
					origin.x += pt.x,origin.y += pt.y
		}
	}
	aLeft   += origin.x
	,aTop    += origin.y
	,aRight  += origin.x
	,aBottom += origin.y

	if (aVariation < 0)
		aVariation := 0
	if (aVariation > 255)
		aVariation := 255

	; Allow colors to vary within the spectrum of intensity, rather than having them
	; wrap around (which doesn't seem to make much sense).  For example, if the user specified
	; a variation of 5, but the red component of aColorBGR is only 0x01, we don't want red_low to go
	; below zero, which would cause it to wrap around to a very intense red color:
	pixel := 0 ; Used much further down.
	;~ search.Fill() ; Used much further down.
	if (aVariation > 0)
	{
		search_red := aColorBGR & 0xFF
		,search_green := aColorBGR >> 8 & 0xFF
		,search_blue := aColorBGR >> 16 & 0xFF
	}
	;else leave uninitialized since they won't be used.
	hdc := DllCall("GetDC","PTR",0,"PTR")
	if (!hdc)
		goto error

	found := false ; Must init here for use by "goto fast_end" and for use by both fast and slow modes.
	if (fast_mode){
		; From this point on, "goto fast_end" will assume hdc is non-NULL but that the below might still be NULL.
		; Therefore, all of the following must be initialized so that the "fast_end" label can detect them:
		sdc := 0
		hbitmap_screen := 0
		screen_pixel := 0
		sdc_orig_select := 0

		; Some explanation for the method below is contained in this quote from the newsgroups:
		; "you shouldn't really be getting the current bitmap from the GetDC DC. This might
		; have weird effects like returning the entire screen or not working. Create yourself
		; a memory DC first of the correct size. Then BitBlt into it and then GetDIBits on
		; that instead. This way, the provider of the DC (the video driver) can make sure that
		; the correct pixels are copied across."

		; Create an empty bitmap to hold all the pixels currently visible on the screen (within the search area):
		search_width := aRight - aLeft + 1
		,search_height := aBottom - aTop + 1
		if !(sdc := DllCall("CreateCompatibleDC","PTR",hdc,"PTR")) || !(hbitmap_screen := DllCall("CreateCompatibleBitmap","PTR",hdc,"UInt", search_width,"UInt", search_height,"PTR"))
			goto fast_end

		if !(sdc_orig_select := DllCall("SelectObject","PTR",sdc,"PTR", hbitmap_screen,"PTR"))
			goto fast_end

		; Copy the pixels in the search-area of the screen into the DC to be searched:
		if !(DllCall("BitBlt","PTR",sdc,"Int", 0,"Int", 0,"Int", search_width,"Int", search_height,"PTR", hdc,"Int", aLeft,"Int", aTop,"Int", SRCCOPY))
			goto fast_end
		;~ _screen_width:=search.screen_width
		;~ _screen_height:=search.screen_height
		if !(screen_pixel := getbitsforimagepixelsearch(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit))
			goto fast_end
		;~ search.screen_height:=_screen_height,search.screen_width:=_screen_width
		; Concerning 0xF8F8F8F8: "On 16bit and 15 bit color the first 5 bits in each byte are valid
		; (in 16bit there is an extra bit but i forgot for which color). And this will explain the
		; second problem [in the test script], since GetPixel even in 16bit will return some "valid"
		; data in the last 3bits of each byte."
		screen_pixel_count := screen_width * screen_height
		if (screen_is_16bit)
			FillBitAnd[screen_pixel_count,screen_pixel[],0xF8F8F8F8]
			;~ Loop % screen_pixel_count
				;~ screen_pixel[i] &= 0xF8F8F8F8
		if (aIsPixelGetColor){
			pt := screen_pixel[0] & 0x00FFFFFF ; See other 0x00FFFFFF below for explanation.
			,found := true ; ErrorLevel will be set to 0 further below.
		}
		else if (aVariation < 1) ; Caller wants an exact match on one particular color.
		{
			if (screen_is_16bit)
				aColorRGB &= 0xF8F8F8F8
			Loop screen_pixel_count{
				; Note that screen pixels sometimes have a non-zero high-order byte.  That's why
				; bit-and with 0x00FFFFFF is done.  Otherwise, reddish/orangish colors are not properly
				; found:
				if ((screen_pixel[A_Index] & 0x00FFFFFF) = aColorRGB){
					found := true,i:=A_Index - 1
					break
				}
			}
		}
		else
		{
			; It seems more appropriate to do the 16-bit conversion prior to SET_COLOR_RANGE,
			; rather than applying 0xF8 to each of the high/low values individually.
			if (screen_is_16bit)
			{
				search_red &= 0xF8
				,search_green &= 0xF8
				,search_blue &= 0xF8
			}

			;~ SET_COLOR_RANGE
			red_low := aVariation > search_red ? 0 : search_red - aVariation
			,green_low := aVariation > search_green ? 0 : search_green - aVariation
			,blue_low := aVariation > search_blue ? 0 : search_blue - aVariation
			,red_high := aVariation > 0xFF - search_red ? 0xFF : search_red + aVariation
			,green_high := aVariation > 0xFF - search_green ? 0xFF : search_green + aVariation
			,blue_high := aVariation > 0xFF - search_blue ? 0xFF : search_blue + aVariation
			Loop screen_pixel_count{
				; Note that screen pixels sometimes have a non-zero high-order byte.  But it doesn't
				; matter with the below approach, since that byte is not checked in the comparison.
				pixel := screen_pixel[A_Index]
				,red := pixel >> 16 & 0xFF   ; Because pixel is in RGB vs. BGR format, red is retrieved with
				,green := pixel >> 8 & 0xFF ; GetBValue() and blue is retrieved with GetRValue().
				,blue := pixel & 0xFF
				if (red >= red_low && red <= red_high
					&& green >= green_low && green <= green_high
					&& blue >= blue_low && blue <= blue_high){
					found := true,i:=A_Index - 1
					break
				}
			}
		}
		goto fast_end
	}
	; Otherwise (since above didn't return): fast_mode==false
	; This old/slower method is kept  because fast mode will break older scripts that rely on
	; which match is found if there is more than one match (since fast mode searches the
	; pixels in a different order (horizontally rather than vertically, I believe).
	; In addition, there is doubt that the fast mode works in all the screen color depths, games,
	; and other circumstances that the slow mode is known to work in.

	; If the caller gives us inverted X or Y coordinates, conduct the search in reverse order.
	; This feature was requested it was put into effect for v1.0.25.06.
	right_to_left := aLeft > aRight
	bottom_to_top := aTop > aBottom

	if (aVariation > 0) {
		;~ SET_COLOR_RANGE
		red_low := aVariation > search_red ? 0 : search_red - aVariation
		,green_low := aVariation > search_green ? 0 : search_green - aVariation
		,blue_low := aVariation > search_blue ? 0 : search_blue - aVariation
		,red_high := aVariation > 0xFF - search_red ? 0xFF : search_red + aVariation
		,green_high := aVariation > 0xFF - search_green ? 0xFF : search_green + aVariation
		,blue_high := aVariation > 0xFF - search_blue ? 0xFF : search_blue + aVariation
	}
	xpos:=aLeft
	While (right_to_left ? xpos >= aRight : xpos <= aRight){
		ypos:=aTop
		While (bottom_to_top ? ypos >= aBottom : ypos <= aBottom){
			pixel := DllCall("GetPixel","PTR",hdc,"Int", xpos,"Int", ypos) ; Returns a BGR value, not RGB. ??? HotKeyIt MSDN says RGB not BGR ???
			if (aVariation < 1)  ; User wanted an exact match.
			{
				if (pixel = aColorBGR)
				{
					found := true
					break
				}
			}
			else  ; User specified that some variation in each of the RGB components is allowable.
			{
				red := pixel & 0xFF
				,green := pixel >> 8 & 0xFF
				,blue := pixel >> 16 & 0xFF
				if (red >= red_low && red <= red_high && green >= green_low && green <= green_high && blue >= blue_low && blue <= blue_high){
					found := true
					break 2
				}
			}
			ypos += bottom_to_top ? -1 : 1
		}
		; Check this here rather than in the outer loop's top line because otherwise the loop's
		; increment would make xpos too big by 1:
		if (found)
			break
		xpos += right_to_left ? -1 : 1
	}

	DllCall("ReleaseDC","PTR",0,"PTR",hdc)
	
	if (!found){
    ErrorLevel:=-1
		return
  }

	; Otherwise, this pixel matches one of the specified color(s).
	; Adjust coords to make them relative to the position of the target window
	; (rect will contain zeroes if this doesn't need to be done):
	;~ pt.x := xpos - origin.x
	x := xpos - origin.x
	;~ pt.y := ypos - origin.y
	y := ypos - origin.y
	; Since above didn't return:
  ErrorLevel:=0
	return

error:
	ErrorLevel:=aIsPixelGetColor ? -1 : -2 ; 2 means error other than "color not found".
  return
fast_end:
		; If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
		; clean up then return.
		DllCall("ReleaseDC","PTR",0,"PTR", hdc)
		if (sdc)
		{
			if (sdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
				DllCall("SelectObject","PTR",sdc,"PTR",sdc_orig_select) ; Probably necessary to prevent memory leak.
			DllCall("DeleteDC","PTR",sdc)
		}
		if (hbitmap_screen)
			DllCall("DeleteObject","PTR",hbitmap_screen)

		if (!found)
			goto error

		; Otherwise, success.  Calculate xpos and ypos of where the match was found and adjust
		; coords to make them relative to the position of the target window (rect will contain
		; zeroes if this doesn't need to be done):

		if (!aIsPixelGetColor)
		{
			;~ if !(pt.x := aLeft + Mod(i,screen_width) - origin.x)
				;~ return -1
			;~ if !(pt.y := aTop + i/screen_width - origin.y)
				;~ return -1
			if !(x := aLeft + Mod(i,screen_width) - origin.x){
				ErrorLevel := -1
        return
      }
			if !(y := aTop + Round(i/screen_width) - origin.y){
				ErrorLevel := -1
        return
      }
		}
    ErrorLevel:=0
		return
}