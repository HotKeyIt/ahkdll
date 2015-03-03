ImageSearch(ByRef x,ByRef y,aLeft, aTop, aRight, aBottom, aImageFile){
; Author: AutoHotkey ImageSearch was created by Aurelian Maga and adapted to AutoHotkey_H by HotKeyIt@autohotkey forums.
	static ICONINFO:="BOOL fIcon;DWORD xHotspot;DWORD yHotspot;HBITMAP hbmMask;HBITMAP hbmColor"
        ,POINT:="x,y"
        ,origin:=Struct(POINT),CLR_NONE:=0xFFFFFFFF,SM_CXSMICON:=49,SM_CYSMICON:=50,IMAGE_ICON:=1,SRCCOPY:=13369376
        ,ii:=Struct(ICONINFO)
				,FillBitAnd:=MCodeH(A_PtrSize=8?"44894424184889542410894C24084883EC18C7042400000000EB088B0424FFC08904248B442420390424732048630424488B4C24288B5424308B048123C248630C24488B54242889048AEBCF4883C418C3":"558BEC83EC44535657C745FC00000000EB098B45FC83C0018945FC8B45FC3B450873178B45FC8B4D0C8B14812355108B45FC8B4D0C891481EBD85F5E5B8BE55DC3","i=uitui")
					/* FillBitAnd DEFINED ABOVE AS	FillBitAnd:=MCodeH(...)
						void FillBitAnd(unsigned int count,unsigned int pixel[],unsigned int value)
						{
							for(int i = 0;i<count;i++)
								pixel[i] &= value;
						}
					*/
				,FindImage:=MCodeH(A_PtrSize=8?"44894C24204C89442418488954241048894C24084883EC58C704240000000083BC24A0000000010F8DFE010000C744240C00000000EB0A8B44240CFFC08944240C8B8424A80000003944240C7D21486344240C488B4C24608B048125FFFFFF0048634C240C488B54246089048AEBC8C744240C00000000EB0A8B44240CFFC08944240C8B8424A80000003944240C0F8D92010000486344240C488B4C2460488B5424688B12391481742648837C247000740A488B4424708338007514488B4424688B8C248800000039080F85510100008B44240C99F7BC24900000008B8C24980000002BC88BC1394424780F8F300100008B44240C99F7BC24900000008BC28B8C24900000002BC88BC1398424800000000F8F0A010000C7042401000000C744240800000000C744240400000000C7442414000000008B44240C89442410EB0A8B442414FFC0894424148B8424B0000000394424140F8DBE000000486344241048634C2414488B5424604C8B442468418B0C88390C82743848837C24700074104863442414488B4C2470833C810075204863442414488B4C24688B942488000000391481740AC744242400000000EB08C7442424010000008B442424890424833C24007502EB528B442408FFC0894424088B842480000000394424087D0C8B442410FFC089442410EB2AC7442408000000008B442404FFC0894424048B4424040FAF8424900000008B4C240C03C88BC189442410E927FFFFFF833C24007402EB05E953FEFFFFE993030000C744240C00000000EB0A8B44240CFFC08944240C8B8424A80000003944240C0F8D6E0300008B44240C99F7BC24900000008B8C24980000002BC88BC1394424780F8F480300008B44240C99F7BC24900000008BC28B8C24900000002BC88BC1398424800000000F8F22030000C7042401000000C744240800000000C744240400000000C7442414000000008B44240C89442410EB0A8B442414FFC0894424148B8424B0000000394424140F8DD60200004863442414488B4C24688B0481C1E8108BC04825FF000000884424204863442414488B4C24680FB70481C1F80848984825FF000000884424184863442414488B4C24688B04814825FF0000008844241D0FB6442420398424A00000007E0AC744242800000000EB100FB64424202B8424A0000000894424280FB6442428884424230FB6442418398424A00000007E0AC744242C00000000EB100FB64424182B8424A00000008944242C0FB644242C8844241F0FB644241D398424A00000007E0AC744243000000000EB100FB644241D2B8424A0000000894424300FB64424308844241A0FB6442420B9FF0000002BC88BC1398424A00000007E0AC7442434FF000000EB100FB6442420038424A0000000894424340FB64424348844241C0FB6442418B9FF0000002BC88BC1398424A00000007E0AC7442438FF000000EB100FB6442418038424A0000000894424380FB6442438884424190FB644241DB9FF0000002BC88BC1398424A00000007E0AC744243CFF000000EB100FB644241D038424A00000008944243C0FB644243C8844241B4863442410488B4C24608B0481C1E8108BC04825FF0000008844241E4863442410488B4C24600FB70481C1F80848984825FF000000884424224863442410488B4C24608B04814825FF000000884424210FB644241E0FB64C24233BC17C460FB644241E0FB64C241C3BC17F380FB64424220FB64C241F3BC17C2A0FB64424220FB64C24193BC17F1C0FB64424210FB64C241A3BC17C0E0FB64424210FB64C241B3BC17E3848837C24700074104863442414488B4C2470833C810075204863442414488B4C24688B942488000000391481740AC744244000000000EB08C7442440010000008B442440890424833C24007502EB528B442408FFC0894424088B842480000000394424087D0C8B442410FFC089442410EB2AC7442408000000008B442404FFC0894424048B4424040FAF8424900000008B4C240C03C88BC189442410E90FFDFFFF833C24007402EB05E977FCFFFF833C2400740A8B44240C89442444EB08C7442444000000008B4424444883C458C3":"558BEC81EC9C0100005356578DBD64FEFFFFB967000000B8CCCCCCCCF3ABC745F800000000837D28010F8D8C010000C745D400000000EB098B45D483C0018945D48B45D43B452C7D1A8B45D48B4D088B148181E2FFFFFF008B45D48B4D08891481EBD5C745D400000000EB098B45D483C0018945D48B45D43B452C0F8D350100008B45D48B4D088B550C8B04813B02741C837D100074088B4510833800750E8B450C8B083B4D1C0F85040100008B45D499F77D208B4D242BC8394D140F8FEF0000008B45D499F77D208B45202BC23945180F8FDA000000C745F801000000C745EC00000000C745E000000000C745C8000000008B45D48945BCEB098B45C883C0018945C88B45C83B45300F8D990000008B45BC8B4D088B55C88B750C8B04813B0496742C837D1000740C8B4DC88B5510833C8A00751A8B45C88B4D0C8B14813B551C740CC78564FEFFFF00000000EB0AC78564FEFFFF010000008B8564FEFFFF8945F8837DF8007502EB3E8B45EC83C0018945EC8B4DEC3B4D187D0B8B45BC83C0018945BCEB1DC745EC000000008B45E083C0018945E08B45E00FAF45200345D48945BCE952FFFFFF837DF8007402EB05E9B6FEFFFFE91F030000C745D400000000EB098B45D483C0018945D48B45D43B452C0F8D010300008B45D499F77D208B4D242BC8394D140F8FE70200008B45D499F77D208B45202BC23945180F8FD2020000C745F801000000C745EC00000000C745E000000000C745C8000000008B45D48945BCEB098B45C883C0018945C88B45C83B45300F8D910200008B45C88B4D0C8B1481C1EA1081E2FF00000088558F8B45C88B4D0C0FB71481C1FA0881E2FF0000008855838B45C88B4D0C8B148181E2FF000000889577FFFFFF0FB6458F3945287E0CC78564FEFFFF00000000EB0D0FB64D8F2B4D28898D64FEFFFF8A9564FEFFFF88956BFFFFFF0FB645833945287E0CC78564FEFFFF00000000EB0D0FB64D832B4D28898D64FEFFFF8A9564FEFFFF88955FFFFFFF0FB68577FFFFFF3945287E0CC78564FEFFFF00000000EB100FB68D77FFFFFF2B4D28898D64FEFFFF8A9564FEFFFF889553FFFFFF0FB6458FB9FF0000002BC8394D287E0CC78564FEFFFFFF000000EB0D0FB6558F035528899564FEFFFF8A8564FEFFFF888547FFFFFF0FB64583B9FF0000002BC8394D287E0CC78564FEFFFFFF000000EB0D0FB65583035528899564FEFFFF8A8564FEFFFF88853BFFFFFF0FB68577FFFFFFB9FF0000002BC8394D287E0CC78564FEFFFFFF000000EB100FB69577FFFFFF035528899564FEFFFF8A8564FEFFFF88852FFFFFFF8B45BC8B4D088B1481C1EA1081E2FF0000008855B38B45BC8B4D080FB71481C1FA0881E2FF0000008855A78B45BC8B4D088B148181E2FF00000088559B0FB645B30FB68D6BFFFFFF3BC17C4B0FB655B30FB68547FFFFFF3BD07F3C0FB64DA70FB6955FFFFFFF3BCA7C2D0FB645A70FB68D3BFFFFFF3BC17F1E0FB6559B0FB68553FFFFFF3BD07C0F0FB64D9B0FB6952FFFFFFF3BCA7E2C837D1000740C8B45C88B4D10833C8100751A8B55C88B450C8B0C903B4D1C740CC78564FEFFFF00000000EB0AC78564FEFFFF010000008B9564FEFFFF8955F8837DF8007502EB3E8B45EC83C0018945EC8B4DEC3B4D187D0B8B45BC83C0018945BCEB1DC745EC000000008B45E083C0018945E08B45E00FAF45200345D48945BCE95AFDFFFF837DF8007402EB05E9EAFCFFFF837DF800740B8B45D4898564FEFFFFEB0AC78564FEFFFF000000008B8564FEFFFF5F5E5B8BE55DC3","i==tttuiuiuiuiuiuiuiui")
				/* IMAGE SEARCH AutoHotkey source code DEFINED ABOVE AS	FindImage:=MCodeH(...)
					int ImageSearch(LPCOLORREF screen_pixel, LPCOLORREF image_pixel, LPCOLORREF image_mask, LONG image_height, LONG image_width, COLORREF trans_color, LONG screen_width, LONG screen_height,int aVariation,int screen_pixel_count,int image_pixel_count)
					{
						int found = 0,x,y,i,j,k;
						if (aVariation < 1)
						{
							for (i = 0; i < screen_pixel_count; ++i)
								screen_pixel[i] &= 0x00FFFFFF;
							for (i = 0; i < screen_pixel_count; ++i)
							{
								if ((screen_pixel[i] == image_pixel[0]
									|| image_mask && image_mask[0]
									|| image_pixel[0] == trans_color)
									&& image_height <= screen_height - i/screen_width
									&& image_width <= screen_width - i%screen_width)
								{
									for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
									{
										if (!(found = (screen_pixel[k] == image_pixel[j]
											|| image_mask && image_mask[j]      
											|| image_pixel[j] == trans_color))) 
											break;
										if (++x < image_width) 
											++k;
										else 
										{
											x = 0; 
											++y;   
											k = i + y*screen_width; 
										}
									}
									if (found) 
										break;
								}
							}
						}
						else 
						{
							BYTE red, green, blue;
							BYTE search_red, search_green, search_blue;
							BYTE red_low, green_low, blue_low, red_high, green_high, blue_high;
							for (i = 0; i < screen_pixel_count; ++i)
							{
								if (image_height <= screen_height - i/screen_width    
									&& image_width <= screen_width - i%screen_width)  
								{
									for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
									{
											search_red = GetBValue(image_pixel[j]);
											search_green = GetGValue(image_pixel[j]);
											search_blue = GetRValue(image_pixel[j]);
										SET_COLOR_RANGE
											red = GetBValue(screen_pixel[k]);
											green = GetGValue(screen_pixel[k]);
											blue = GetRValue(screen_pixel[k]);
										if (!(found = red >= red_low && red <= red_high
											&& green >= green_low && green <= green_high
																	&& blue >= blue_low && blue <= blue_high
												|| image_mask && image_mask[j]     
												|| image_pixel[j] == trans_color)) 
											break; 
										if (++x < image_width) 
											++k;
										else 
										{
											x = 0; 
											++y;   
											k = i + y*screen_width; 
										}
									}
									if (found) 
										break;
								}
							}
						}
						return found ? i : 0;
					}
					*/
	; Many of the following sections are similar to those in PixelSearch(), so they should be
	; maintained together.
	
	origin.Fill(),pt.Fill()
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
	,aLeft   += origin.x
	,aTop    += origin.y
	,aRight  += origin.x
	,aBottom += origin.y

	; Options are done as asterisk+option to permit future expansion.
	; Set defaults to be possibly overridden by any specified options:
	,aVariation := 0  ; This is named aVariation vs. variation for use with the SET_COLOR_RANGE macro.
	,trans_color := CLR_NONE ; The default must be a value that can't occur naturally in an image.
	,icon_number := 0 ; Zero means "load icon or bitmap (doesn't matter)".
	,width := 0, height := 0
	; For icons, override the default to be 16x16 because that is what is sought 99% of the time.
	; This new default can be overridden by explicitly specifying w0 h0:
	,ext := SubStr(aImageFile,InStr(aImageFile,".",1,-1)+1)
	if (ext="ico" || ext="exe" || ext="dll")
			width := DllCall("GetSystemMetrics","Int",SM_CXSMICON), height := DllCall("GetSystemMetrics","Int",SM_CYSMICON)
	
	;~ TCHAR color_name[32], *dp
	;~ cp = omit_leading_whitespace(aImageFile) ; But don't alter aImageFile yet in case it contains literal whitespace we want to retain.
	LoopParse,%aImageFile%,%A_Space%%A_Tab%
	{
		If !A_LoopField ; multiple A_Space or A_Tab
			continue
		else if InStr(A_LoopField,"icon"){
			icon_number := SubStr(A_LoopField,5)
		} else if InStr(A_LoopField,"w"){
			width := SubStr(A_LoopField,2)
		} else if InStr(A_LoopField,"h"){
			height := SubStr(A_LoopField,2)
		} else if InStr(A_LoopField,"trans"){
			If ((trans_color:=ColorNameToBGR(SubStr(A_LoopField,6))) = CLR_NONE)
				trans_color:=SubStr(A_LoopField,6)
		} else if (Type(A_LoopField)="Integer"){
			if (A_LoopField < 0)
					aVariation := 0
				if (A_LoopField > 255)
					aVariation := 255
		} else
			aFileName .= A_LoopField A_Space ; ensure last entry will be the file name (append A_Space because it cannot contain Tabs)
	}
	aFileName:=trim(aFileName) ; we apended 1 space to much

	; Update: Transparency is now supported in icons by using the icon's mask.  In addition, an attempt
	; is made to support transparency in GIF, PNG, and possibly TIF files via the *Trans option, which
	; assumes that one color in the image is transparent.  In GIFs not loaded via GDIPlus, the transparent
	; color might always been seen as pure white, but when GDIPlus is used, it's probably always black
	; like it is in PNG -- however, this will not relied upon, at least not until confirmed.
	; OLDER/OBSOLETE comment kept for background:
	; For now, images that can't be loaded as bitmaps (icons and cursors) are not supported because most
	; icons have a transparent background or color present, which the image search routine here is
	; probably not equipped to handle (since the transparent color, when shown, typically reveals the
	; color of whatever is behind it thus screen pixel color won't match image's pixel color).
	; So currently, only BMP and GIF seem to work reliably, though some of the other GDIPlus-supported
	; formats might work too.
	
	hbitmap_image := LoadPicture(aImageFile, width, height, icon_number, image_type, false)
	
	; The comment marked OBSOLETE below is no longer true because the elimination of the high-byte via
	; 0x00FFFFFF seems to have fixed it.  But "true" is still not passed because that should increase
	; consistency when GIF/BMP/ICO files are used by a script on both Win9x and other OSs (since the
	; same loading method would be used via "false" for these formats across all OSes).
	; OBSOLETE: Must not pass "true" with the above because that causes bitmaps and gifs to be not found
	; by the search.  In other words, nothing works.  Obsolete comment: Pass "true" so that an attempt
	; will be made to load icons as bitmaps if GDIPlus is available.
	if (!hbitmap_image)
		return -1

	if !(hdc := DllCall("GetDC","PTR",0,"PTR"))
		return DllCall("DeleteObject","PTR",hbitmap_image),-1

	; From this point on, "goto end" will assume hdc and hbitmap_image are non-NULL, but that the below
	; might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	; label can detect them:
	;~ sdc := 0
	;~ ,hbitmap_screen := 0
	;~ ,image_pixel := 0, screen_pixel := 0, image_mask := 0
	;~ ,sdc_orig_select := 0
	found := false ; Must init here for use by "goto end".
    
	;~ ,image_is_16bit := 0
	;~ ,image_width := 0, image_height := 0
	if (image_type = IMAGE_ICON)
	{
		; Must be done prior to IconToBitmap() since it deletes (HICON)hbitmap_image:
		ii.Fill()
		if (DllCall("GetIconInfo","PTR",hbitmap_image,"PTR", ii[]))
		{
			; If the icon is monochrome (black and white), ii.hbmMask will contain twice as many pixels as
			; are actually in the icon.  But since the top half of the pixels are the AND-mask, it seems
			; okay to get all the pixels given the rarity of monochrome icons.  This scenario should be
			; handled properly because: 1) the variables image_height and image_width will be overridden
			; further below with the correct icon dimensions 2) Only the first half of the pixels within
			; the image_mask array will actually be referenced by the transparency checker in the loops,
			; and that first half is the AND-mask, which is the transparency part that is needed.  The
			; second half, the XOR part, is not needed and thus ignored.  Also note that if width/height
			; required the icon to be scaled, LoadPicture() has already done that directly to the icon,
			; so ii.hbmMask should already be scaled to match the size of the bitmap created later below.
			image_mask := getbitsforimagepixelsearch(ii.hbmMask, hdc, image_width, image_height, image_is_16bit, 1)
			,DllCall("DeleteObject","PTR",ii.hbmColor) ; DeleteObject() probably handles NULL okay since few MSDN/other examples ever check for NULL.
			,DllCall("DeleteObject","PTR",ii.hbmMask)
		}
		if !(hbitmap_image := IconToBitmap(hbitmap_image, true))
			return -1
	}
	
	if !(image_pixel := getbitsforimagepixelsearch(hbitmap_image, hdc, image_width, image_height, image_is_16bit)){
		DllCall("ReleaseDC","PTR",0,"PTR", hdc)
  	DllCall("DeleteObject","PTR",hbitmap_image)
    return -1
  }
	; Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	search_width := aRight - aLeft + 1
	search_height := aBottom - aTop + 1
	;~ MsgBox ;% search_height "-" search_width
	;~ VarSetCapacity(a,100)
	if (!(sdc := DllCall("CreateCompatibleDC","PTR",hdc,"PTR")) || !(hbitmap_screen := DllCall("CreateCompatibleBitmap","PTR",hdc,"Int", search_width,"Int", search_height,"PTR"))
      || !(sdc_orig_select := DllCall("SelectObject","PTR",sdc,"PTR", hbitmap_screen,"PTR"))){
		DllCall("ReleaseDC","PTR",0,"PTR", hdc)
  	DllCall("DeleteObject","PTR",hbitmap_image)
  	if (sdc)
  	{
  		if (sdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
  			DllCall("SelectObject","PTR",sdc,"PTR", sdc_orig_select) ; Probably necessary to prevent memory leak.
  		DllCall("DeleteDC","PTR",sdc)
  	}
  	if (hbitmap_screen)
  		DllCall("DeleteObject","PTR",hbitmap_screen)
    return -1
  }
	; Copy the pixels in the search-area of the screen into the DC to be searched:
	if (!DllCall("BitBlt","PTR",sdc,"Int", 0,"Int", 0,"Int", search_width,"Int", search_height,"PTR", hdc,"Int", aLeft,"Int", aTop,"Int", SRCCOPY)
		|| !(screen_pixel := getbitsforimagepixelsearch(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit))){
    DllCall("ReleaseDC","PTR",0,"PTR", hdc)
  	DllCall("DeleteObject","PTR",hbitmap_image)
  	if (sdc)
  	{
  		if (sdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
  			DllCall("SelectObject","PTR",sdc,"PTR", sdc_orig_select) ; Probably necessary to prevent memory leak.
  		DllCall("DeleteDC","PTR",sdc)
  	}
  	if (hbitmap_screen)
  		DllCall("DeleteObject","PTR",hbitmap_screen)
    return -1
  }
	image_pixel_count := image_width * image_height
	,screen_pixel_count := screen_width * screen_height
	;,i:=0, j:=0, k:=0, x:=0, y:=0 ; Declaring as "register" makes no performance difference with current compiler, so let the compiler choose which should be registers.
	
	; If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if (image_is_16bit || screen_is_16bit)
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8 ; Convert indicated trans-color to be compatible with the conversion below.
		
		FillBitAnd[screen_pixel_count - 1,screen_pixel[],0xF8F8F8F8]
		;~ Loop % screen_pixel_count - 1
			;~ screen_pixel[A_Index] &= 0xF8F8F8F8 ; Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
		
		FillBitAnd[image_pixel_count - 1,image_pixel_count[],0xF8F8F8F8]
		;~ Loop % image_pixel_count - 1
			;~ image_pixel[A_Index] &= 0xF8F8F8F8  ; Same.
	}

	; v1.0.44.03: The below is now done even for variation>0 mode so its results are consistent with those of
	; non-variation mode.  This is relied upon by variation=0 mode but now also by the following line in the
	; variation>0 section:
	;     || image_pixel[j] == trans_color
	; Without this change, there are cases where variation=0 would find a match but a higher variation
	; (for the same search) wouldn't. 
	FillBitAnd[image_pixel_count - 1,image_pixel[],0x00FFFFFF]
	;~ Loop % image_pixel_count - 1
		;~ image_pixel[A_Index] &= 0x00FFFFFF
		
	If i:=FindImage[screen_pixel[], image_pixel[], image_mask?image_mask[]:0, image_height, image_width, trans_color, screen_width, screen_height,aVariation,screen_pixel_count,image_pixel_count]
		found:=true

	; If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
	; clean up then return.
	DllCall("ReleaseDC","PTR",0,"PTR", hdc)
	DllCall("DeleteObject","PTR",hbitmap_image)
	if (sdc)
	{
		if (sdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
			DllCall("SelectObject","PTR",sdc,"PTR", sdc_orig_select) ; Probably necessary to prevent memory leak.
		DllCall("DeleteDC","PTR",sdc)
	}
	if (hbitmap_screen)
		DllCall("DeleteObject","PTR",hbitmap_screen)

	if (!found) ; Let ErrorLevel, which is either "1" or "2" as set earlier, tell the story.
		return -1

	; Otherwise, success.  Calculate xpos and ypos of where the match was found and adjust
	; coords to make them relative to the position of the target window (rect will contain
	; zeroes if this doesn't need to be done):
	;~ if (output_var_x && !output_var_x->Assign())
		;~ return FAIL
	;~ if (output_var_y && !output_var_y->Assign((aTop + i/screen_width) - origin.y))
		;~ return FAIL
	x:=(aLeft + Mod(i,screen_width)) - origin.x,y:=(aTop + Round(i/screen_width)) - origin.y
	return found ;Struct(POINT,{x:(aLeft + Mod(i,screen_width)) - origin.x,y:(aTop + i/screen_width) - origin.y}) ;g_ErrorLevel->Assign(ERRORLEVEL_NONE) ; Indicate success.
}
