getbitsforimagepixelsearch(ahImage, hdc, ByRef aWidth, ByRef aHeight, ByRef aIs16Bit, aMinColorDepth := 8){
; Helper function used by PixelSearch below.
; Returns an array of pixels to the caller, which it must free when done.  Returns NULL on failure,
; in which case the contents of the output parameters is indeterminate.
	static BITMAPINFOHEADER:="DWORD biSize;LONG biWidth;LONG biHeight;WORD biPlanes;WORD biBitCount;DWORD biCompression;DWORD biSizeImage;LONG biXPelsPerMeter;LONG biYPelsPerMeter;DWORD biClrUsed;DWORD biClrImportant"
				,PALETTEENTRY:="BYTE peRed;BYTE peGreen;BYTE peBlue;BYTE peFlags"
        ,RGBQUAD:="BYTE rgbBlue;BYTE rgbGreen;BYTE rgbRed;BYTE rgbReserved"
				,_BITMAPINFO3:="getbitsforimagepixelsearch(BITMAPINFOHEADER) bmiHeader;getbitsforimagepixelsearch(RGBQUAD) bmiColors[260]",COLORREF:="DWORD"
        ,bmi:=Struct(_BITMAPINFO3),DIB_RGB_COLORS:=0
	if !(tdc := DllCall("CreateCompatibleDC","PTR",hdc,"PTR"))
		return 0

	; From this point on, "goto end" will assume tdc is non-NULL, but that the below
	; might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	; label can detect them:
	tdc_orig_select := 0
	,image_pixel := 0
	,success := false

	; Confirmed:
	; Needs extra memory to prevent buffer overflow due to: "A bottom-up DIB is specified by setting
	; the height to a positive number, while a top-down DIB is specified by setting the height to a
	; negative number. THE BITMAP COLOR TABLE WILL BE APPENDED to the BITMAPINFO structure."
	; Maybe this applies only to negative height, in which case the second call to GetDIBits()
	; below uses one.
	,bmi.Fill() ; free previously used values
	,bmi.bmiHeader.biSize := sizeof(BITMAPINFOHEADER)
	,bmi.bmiHeader.biBitCount := 0 ; i.e. "query bitmap attributes" only.
	if (!DllCall("GetDIBits","PTR",tdc,"PTR", ahImage,"UInt", 0,"UInt", 0,"PTR", 0,"PTR", bmi[],"UInt", DIB_RGB_COLORS)
		|| bmi.bmiHeader.biBitCount < aMinColorDepth){ ; Relies on short-circuit boolean order.
  	DllCall("DeleteDC","PTR",tdc)
  	return
  }
	
	; Set output parameters for caller:
	
	aIs16Bit := (bmi.bmiHeader.biBitCount = 16)
	,aWidth := bmi.bmiHeader.biWidth
	,aHeight := bmi.bmiHeader.biHeight
	,image_pixel_count := aWidth * aHeight
	
	if !(image_pixel :=Struct("COLORREF[" image_pixel_count * sizeof(COLORREF) "]")){
  	DllCall("DeleteDC","PTR",tdc)
  	return
	}
	
	; v1.0.40.10: To preserve compatibility with callers who check for transparency in icons, don't do any
	; of the extra color table handling for 1-bpp images.  Update: For code simplification, support only
	; 8-bpp images.  If ever support lower color depths, use something like "bmi.bmiHeader.biBitCount > 1
	; && bmi.bmiHeader.biBitCount < 9"
	if !(is_8bit := (bmi.bmiHeader.biBitCount = 8))
		bmi.bmiHeader.biBitCount := 32
	bmi.bmiHeader.biHeight := -bmi.bmiHeader.biHeight ; Storing a negative inside the bmiHeader struct is a signal for GetDIBits().

	; Must be done only after GetDIBits() because: "The bitmap identified by the hbmp parameter
	; must not be selected into a device context when the application calls GetDIBits()."
	; (Although testing shows it works anyway, perhaps because GetDIBits() above is being
	; called in its informational mode only).
	; Note that this seems to return NULL sometimes even though everything still works.
	; Perhaps that is normal.
	tdc_orig_select := DllCall("SelectObject","PTR",tdc,"PTR", ahImage,"PTR") ; Returns NULL when we're called the second time?

	; Apparently there is no need to specify DIB_PAL_COLORS below when color depth is 8-bit because
	; DIB_RGB_COLORS also retrieves the color indices.
	;~ VarSetCapacity(pixel,image_pixel_count * CRFsize)
	if !DllCall("GetDIBits","PTR",tdc,"PTR", ahImage,"UInt", 0,"UInt", aHeight,"PTR", image_pixel[], "PTR", bmi[],"UInt", DIB_RGB_COLORS){
		if (tdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
  		DllCall("SelectObject","PTR",tdc,"PTR",tdc_orig_select) ; Probably necessary to prevent memory leak.
  	DllCall("DeleteDC","PTR",tdc)
  	if (!success && image_pixel)
  		image_pixel:=""
  	return image_pixel
  }
	
	if (is_8bit) ; This section added in v1.0.40.10.
	{
		; Convert the color indices to RGB colors by going through the array in reverse order.
		; Reverse order allows an in-place conversion of each 8-bit color index to its corresponding
		; 32-bit RGB color.
		palette:=Struct("PALETTEENTRY[256]")
		,DllCall("GetSystemPaletteEntries","PTR",tdc,"UInt", 0,"UInt", 256,"PTR", palette[]) ; Even if failure can realistically happen, consequences of using uninitialized palette seem acceptable.
		; Above: GetSystemPaletteEntries() is the only approach that provided the correct palette.
		; The following other approaches didn't give the right one:
		; GetDIBits(): The palette it stores in bmi.bmiColors seems completely wrong.
		; GetPaletteEntries()+GetCurrentObject(hdc, OBJ_PAL): Returned only 20 entries rather than the expected 256.
		; GetDIBColorTable(): I think same as above or maybe it returns 0.

		; The following section is necessary because apparently each new row in the region starts on
		; a DWORD boundary.  So if the number of pixels in each row isn't an exact multiple of 4, there
		; are between 1 and 3 zero-bytes at the end of each row.
		,remainder := Mod(aWidth,4) ;aWidth % 4
		,empty_bytes_at_end_of_each_row := remainder ? (4 - remainder) : 0

		; Start at the last RGB slot and the last color index slot:
		,byte := image_pixel[] + image_pixel_count - 1 + (aHeight * empty_bytes_at_end_of_each_row) ; Pointer to 8-bit color indices.
		;BYTE *byte = (BYTE *)image_pixel + image_pixel_count - 1 + (aHeight * empty_bytes_at_end_of_each_row) ; Pointer to 8-bit color indices.
		,pixel := image_pixel[] + image_pixel_count - 1
		;DWORD *pixel = image_pixel + image_pixel_count - 1 ; Pointer to 32-bit RGB entries.

		Loop % aHeight ; For each row.
		{
			byte -= empty_bytes_at_end_of_each_row
			Loop % aWidth
				Color:=palette[NumGet(byte--,"Char")],NumPut(((Color&255)<<16) + (((Color>>8)&255)<<8) + (Color>>16),pixel--,"UInt")
				;pixel-- := rgb_to_bgr(palette[NumGet(byte--,"Char")]) ; Caller always wants RGB vs. BGR format.
		}
	}
	
	; Since above didn't return, indicate success:
	success := true
	if (tdc_orig_select) ; i.e. the original call to SelectObject() didn't fail.
		DllCall("SelectObject","PTR",tdc,"PTR",tdc_orig_select) ; Probably necessary to prevent memory leak.
	DllCall("DeleteDC","PTR",tdc)
	if (!success && image_pixel)
		image_pixel:=""
	return image_pixel
}
