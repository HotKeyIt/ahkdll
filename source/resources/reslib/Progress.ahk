Progress_Struct(){
	static MAX_PROGRESS_WINDOWS:=10,SplashType:="int width;int height;int bar_pos;int margin_x;int margin_y;int text1_height;int object_width;int object_height;HWND hwnd;int pic_type;union{HBITMAP pic_bmp;HICON pic_icon};HWND hwnd_bar;HWND hwnd_text1;HWND hwnd_text2;HFONT hfont1;HFONT hfont2;HBRUSH hbrush;COLORREF color_bk;COLORREF color_text"
				,g_Progress:=Struct("Progress_Struct(SplashType)[" MAX_PROGRESS_WINDOWS "]")
	return g_progress
}
Progress_OnMessage(wParam,lParam,msg,hwnd){
	static MAX_SPLASHIMAGE_WINDOWS:=10,MAX_PROGRESS_WINDOWS:=10,SW_SHOWNOACTIVATE:=4,WM_SETTEXT:=12
				,WS_DISABLED:=134217728,WS_POPUP:=2147483648,WS_CAPTION:=12582912,WS_EX_TOPMOST:=8,COORD_UNSPECIFIED:=(-2147483647 - 1)
				,WS_SIZEBOX:=262144,WS_MINIMIZEBOX:=131072,WS_MAXIMIZEBOX:=65536,WS_SYSMENU:=524288,LOGPIXELSY:=90,IMAGE_BITMAP:=0
				,FW_DONTCARE:=0,CLR_DEFAULT:=4278190080,CLR_NONE:=4294967295,DEFAULT_GUI_FONT:=17,IMAGE_ICON:=1,SS_LEFT:=0
				,FW_SEMIBOLD:=600, DEFAULT_CHARSET:=1, OUT_TT_PRECIS:=4, CLIP_DEFAULT_PRECIS:=0, PROOF_QUALITY:=2,FF_DONTCARE:=0
				,DT_CALCRECT:=1024, DT_WORDBREAK:=16, DT_EXPANDTABS:=64,SPI_GETWORKAREA:=48,IDI_MAIN:=159,LR_SHARED:=32768,ICON_SMALL:=0,ICON_BIG:=1
				,WS_CHILD:=1073741824,WS_VISIBLE:=268435456,SS_NOPREFIX:=0x80,SS_CENTER:=1,SS_LEFTNOWORDWRAP:=12,PBM_GETPOS:=1032,PBM_SETPOS:=1026
				,PBM_SETRANGE:=1025,PBM_SETRANGE32:=1030,PROGRESS_CLASS:="msctls_progress32",WS_EX_CLIENTEDGE:=512,PBS_SMOOTH:=1,WM_PAINT:=15,WM_SIZE:=5
				,PBM_SETBARCOLOR:=1033,PBM_SETBKCOLOR:=8193,WM_SETFONT:=48,SRCCOPY:=13369376,DI_NORMAL:=3,COLOR_BTNFACE:=15,WM_ERASEBKGND:=20,WM_CTLCOLORSTATIC:=312
				,Black:=0,Silver:=0xC0C0C0,Gray:=0x808080,White:=0xFFFFFF,Maroon:=0x000080,Red:=0x0000FF
				,Purple:=0x800080,Fuchsia:=0xFF00FF,Green:=0x008000,Lime:=0x00FF00,Olive:=0x008080
				,Yellow:=0x00FFFF,Navy:=0x800000,Blue:=0xFF0000,Teal:=0x808000,Aqua:=0xFFFF00,Default:=CLR_DEFAULT
				;~ ,WNDCLASSEX:="UINT cbSize;UINT style;WNDPROC lpfnWndProc;int cbClsExtra;int cbWndExtra;HINSTANCE hInstance;HICON hIcon;HCURSOR hCursor;HBRUSH hbrBackground;LPCWSTR lpszMenuName;LPCWSTR lpszClassName;HICON hIconSm"
				;~ ,ws:=Struct("Splash(WNDCLASSEX)",{cbSize:sizeof("Splash(WNDCLASSEX)")})
				,g_SplashImage:=SplashImage_Struct(),RECT:="LONG left,LONG top,LONG right,LONG bottom",client_rect:=Struct(RECT), draw_rect:=Struct(RECT), main_rect:=Struct(RECT), work_rect:=Struct(RECT)
	;~ Sleep 1000
	Loop % MAX_PROGRESS_WINDOWS
		if (g_Progress[i:=A_Index].hwnd == hwnd)
			break
	if (i == MAX_PROGRESS_WINDOWS){ ; It's not a progress window either.
		; Let DefWindowProc() handle it (should probably never happen since currently the only
		; other type of window is SplashText, which never receive this msg?)
		onmsg:=OnMessage(msg,"")
		ret:=DllCall("SendMessage","Ptr",hwnd,"UInt",msg,"PTR",wParam,"PTR",lParam,"PTR")
		OnMessage(msg,onmsg)
		Return ret
	}
	splash := g_Progress[i]
	If (msg=WM_SIZE){
		new_width := lParam & 0xFFFF
		new_height := (lParam>>16) & 0xFFFF
		if (new_width != splash.width || new_height != splash.height)
		{
			DllCall("GetClientRect","PTR",splash.hwnd,"PTR", client_rect[])
			control_width := client_rect.right - (splash.margin_x * 2)
			bar_y := splash.margin_y + (splash.text1_height ? (splash.text1_height + splash.margin_y) : 0)
			sub_y := bar_y + splash.object_height + (splash.object_height ? splash.margin_y : 0)  ;  Calculate the Y position of each control in the window.
			; The Y offset for each control should match those used in Splash():
			if (new_width != splash.width)
			{
				if (splash.hwnd_text1) ; This control doesn't exist if the main text was originally blank.
					DllCall("MoveWindow","PTR",splash.hwnd_text1,"Int", splash.margin_x,"Int", splash.margin_y,"Int", control_width,"Int", splash.text1_height,"Int", FALSE)
				if (splash.hwnd_bar)
					DllCall("MoveWindow","PTR",splash.hwnd_bar,"Int", splash.margin_x,"Int", bar_y,"Int", control_width,"Int", splash.object_height,"Int", FALSE)
				splash.width := new_width
			}
			; Move the window EVEN IF new_height == splash.height because otherwise the text won't
			; get re-centered when only the width of the window changes:
			DllCall("MoveWindow","PTR",splash.hwnd_text2,"Int", splash.margin_x,"Int", sub_y,"Int", control_width,"Int", (client_rect.bottom - client_rect.top) - sub_y,"Int", FALSE) ; Negative height seems handled okay.
			; Specifying true for "repaint" in MoveWindow() is not always enough refresh the text correctly,
			; so this is done instead:
			DllCall("InvalidateRect","PTR",splash.hwnd,"PTR", client_rect[],"Int", TRUE)
			; If the user resizes the window, have that size retained (remembered) until the script
			; explicitly changes it or the script destroys the window.
			splash.height := new_height
		}
		return 0 ; i.e. completely handled here.
	}	else if (msg=WM_CTLCOLORSTATIC){
		if (!splash.hbrush && splash.color_text == CLR_DEFAULT) ; Let DWP handle it.
			Return DllCall("DefWindowProc","PTR",hWnd,"UInt", Msg,"PTR", wParam,"PTR", lParam,"PTR")
		; Since we're processing this msg and not DWP, must set background color unconditionally,
		; otherwise plain white will likely be used:
		DllCall("SetBkColor","PTR",wParam,"UInt", splash.hbrush ? splash.color_bk : DllCall("GetSysColor","Int",COLOR_BTNFACE))
		if (splash.color_text != CLR_DEFAULT)
			DllCall("SetTextColor","PTR",wParam,"UInt", splash.color_text)
		; Always return a real HBRUSH so that Windows knows we altered the HDC for it to use:
		return splash.hbrush ? splash.hbrush : DllCall("GetSysColorBrush","Int",COLOR_BTNFACE,"PTR")
	} else if (msg=WM_ERASEBKGND){
		if (splash.pic_bmp) ; And since there is a pic, its object_width/height should already be valid.
		{
			ypos := splash.margin_y + (splash.text1_height ? (splash.text1_height + splash.margin_y) : 0)
			if (splash.pic_type == IMAGE_BITMAP)
			{
				hdc := DllCall("CreateCompatibleDC","PTR",wParam,"PTR")
				,hbmpOld := DllCall("SelectObject","PTR",hdc,"PTR", splash.pic_bmp,"PTR")
				,DllCall("BitBlt","PTR",wParam,"Int", splash.margin_x,"Int", ypos,"Int", splash.object_width,"Int", splash.object_height,"PTR", hdc,"Int", 0,"Int", 0,"Int", SRCCOPY)
				,DllCall("SelectObject","PTR",hdc,"PTR", hbmpOld)
				,DllCall("DeleteDC","PTR",hdc)
			}
			else ; IMAGE_ICON
				DllCall("DrawIconEx","PTR",wParam,"Int", splash.margin_x,"Int", ypos,"Int", splash.pic_icon,"Int", splash.object_width,"Int", splash.object_height,"ptr", 0,"Int", 0,"Int", DI_NORMAL)
			; Prevent "flashing" by erasing only the part that hasn't already been drawn:
			DllCall("ExcludeClipRect","PTR",wParam,"Int", splash.margin_x,"Int", ypos,"Int", splash.margin_x + splash.object_width,"Int", ypos + splash.object_height)
			,hrgn := DllCall("CreateRectRgn","Int", 0,"Int", 0,"Int", 1,"Int", 1,"PTR")
			,DllCall("GetClipRgn","PTR",wParam,"PTR",hrgn)
			,DllCall("FillRgn","PTR",wParam,"PTR", hrgn,"PTR", splash.hbrush ? splash.hbrush : DllCall("GetSysColorBrush","Int",COLOR_BTNFACE))
			,DllCall("DeleteObject","PTR",hrgn)
			return 1
		}
		; Otherwise, it's a Progress window (or a SplashImage window with no picture):
		if (splash.hbrush){ ; Let DWP handle it.
			DllCall("GetClipBox","PTR",hdc,"PTR", client_rect[])
			DllCall("FillRect","PTR",hdc, client_rect[],"PTR", splash.hbrush)
		}
	}
	;~ DllCall("InvalidateRect","PTR",splash.hwnd,"PTR", NULL,"Int", TRUE)
	return DllCall("DefWindowProc","PTR",hWnd,"UInt", Msg,"PTR", wParam,"PTR", lParam,"PTR") ;ret
}
Progress(aOptions:="",aSubText:="", aMainText:="", aTitle:="",aFontName:=""){
	static MAX_SPLASHIMAGE_WINDOWS:=10,MAX_PROGRESS_WINDOWS:=10,SW_SHOWNOACTIVATE:=4,WM_SETTEXT:=12
				,WS_DISABLED:=134217728,WS_POPUP:=2147483648,WS_CAPTION:=12582912,WS_EX_TOPMOST:=8,COORD_UNSPECIFIED:=(-2147483647 - 1)
				,WS_SIZEBOX:=262144,WS_MINIMIZEBOX:=131072,WS_MAXIMIZEBOX:=65536,WS_SYSMENU:=524288,LOGPIXELSX:=88,LOGPIXELSY:=90,IMAGE_BITMAP:=0
				,FW_DONTCARE:=0,CLR_NONE:=4294967295,DEFAULT_GUI_FONT:=17,IMAGE_ICON:=1,CLR_DEFAULT:=4278190080
				,SPLASH_DEFAULT_WIDTH:=DllCall("MulDiv","Int",300,"Int", DllCall("GetDeviceCaps","PTR",hdc:=DllCall("GetDC","PTR",0,"PTR"),"Int",LOGPIXELSX),"Int",96),rel:=DllCall("ReleaseDC","PTR",0,"PTR",hdc)
				, FW_SEMIBOLD:=600, DEFAULT_CHARSET:=1, OUT_TT_PRECIS:=4, CLIP_DEFAULT_PRECIS:=0, PROOF_QUALITY:=2,FF_DONTCARE:=0,SS_LEFT:=0
				,DT_CALCRECT:=1024, DT_WORDBREAK:=16, DT_EXPANDTABS:=64,SPI_GETWORKAREA:=48,IDI_MAIN:=159,LR_SHARED:=32768,ICON_SMALL:=0,ICON_BIG:=1
				,WS_CHILD:=1073741824,WS_VISIBLE:=268435456,SS_NOPREFIX:=0x80,SS_CENTER:=1,SS_LEFTNOWORDWRAP:=12,PBM_GETPOS:=1032,PBM_SETPOS:=1026
				,PBM_SETRANGE:=1025,PBM_SETRANGE32:=1030,PROGRESS_CLASS:="msctls_progress32",WS_EX_CLIENTEDGE:=512,PBS_SMOOTH:=1,WM_PAINT:=15,WM_SIZE:=5
				,PBM_SETBARCOLOR:=1033,PBM_SETBKCOLOR:=8193,WM_SETFONT:=48,SRCCOPY:=13369376,DI_NORMAL:=3,COLOR_BTNFACE:=15,WM_ERASEBKGND:=20,WM_CTLCOLORSTATIC:=312
				,Black:=0,Silver:=0xC0C0C0,Gray:=0x808080,White:=0xFFFFFF,Maroon:=0x000080,Red:=0x0000FF
				,Purple:=0x800080,Fuchsia:=0xFF00FF,Green:=0x008000,Lime:=0x00FF00,Olive:=0x008080
				,Yellow:=0x00FFFF,Navy:=0x800000,Blue:=0xFF0000,Teal:=0x808000,Aqua:=0xFFFF00,Default:=CLR_DEFAULT
				;~ ,WNDCLASSEX:="UINT cbSize;UINT style;WNDPROC lpfnWndProc;int cbClsExtra;int cbWndExtra;HINSTANCE hInstance;HICON hIcon;HCURSOR hCursor;HBRUSH hbrBackground;LPCWSTR lpszMenuName;LPCWSTR lpszClassName;HICON hIconSm"
				;~ ,ws:=Struct("Splash(WNDCLASSEX)",{cbSize:sizeof("Splash(WNDCLASSEX)")})
				,g_Progress:=Progress_Struct()
				,tm:=Struct("LONG tmHeight;LONG tmAscent;LONG tmDescent;LONG tmInternalLeading;LONG tmExternalLeading;LONG tmAveCharWidth;LONG tmMaxCharWidth;LONG tmWeight;LONG tmOverhang;LONG tmDigitizedAspectX;LONG tmDigitizedAspectY;WCHAR tmFirstChar;WCHAR tmLastChar;WCHAR tmDefaultChar;WCHAR tmBreakChar;BYTE tmItalic;BYTE tmUnderlined;BYTE tmStruckOut;BYTE tmPitchAndFamily;BYTE tmCharSet") ;TEXTMETRICW
				,iconinfo:=Struct("BOOL fIcon;DWORD xHotspot;DWORD yHotspot;HBITMAP hbmMask;HBITMAP hbmColor") ;ICONINFO
				,RECT:="LONG left,LONG top,LONG right,LONG bottom",client_rect:=Struct(RECT), draw_rect:=Struct(RECT), main_rect:=Struct(RECT), work_rect:=Struct(RECT)
				,bmp:=Struct("LONG bmType;LONG bmWidth;LONG bmHeight;LONG bmWidthBytes;WORD bmPlanes;WORD bmBitsPixel;LPVOID bmBits") ;BITMAP
				,initGui:=Gui("SPLASH_GUI_Init:Show","HIDE") Gui("SPLASH_GUI_Init:Destroy") ; required to init ahk_class AutoHotkeyGUI
				,_ttoi:=DynaCall("msvcrt\_wtoi","t==t")
	ErrorLevel := 0    ; Set default
	window_index := 1  ;  Set the default window to operate upon (the first).
	turn_off := false
	show_it_only := false
	bar_pos_has_been_set := false
	options_consist_of_bar_pos_only := false
  if !InStr(aOptions, ":")
    options := aOptions
  else
  {
    window_index := SubStr(aOptions,1,InStr(aOptions,":")-1)
    if (window_index < 0 || window_index >= MAX_PROGRESS_WINDOWS){
			MsgBox,0,Error in Function %A_ThisFunc%,% "Max window number is " MAX_PROGRESS_WINDOWS "."
      ErrorLevel:=-1
      return
    }
		options:=SubStr(aOptions,InStr(aOptions,":")+1)
  }
  options := LTrim(options) ;  Added in v1.0.38.04 per someone's suggestion.
  if (options="Off")
          turn_off := true
  else if (options="Show")
    show_it_only := true
  else
  {
    ;  Allow floats at runtime for flexibility (i.e. in case aOptions was in a variable reference).
    ;  But still use ATOI for the conversion:
    if (options+0!="") ;  Negatives are allowed as of v1.0.25.
    {
      bar_pos := options+0
      bar_pos_has_been_set := true
      options_consist_of_bar_pos_only := true
    }
    ; else leave it set to the default.
  }
	splash := g_Progress[window_index]
	;  In case it's possible for the window to get destroyed by other means (WinClose?).
	;  Do this only after the above options were set so that the each window's settings
	;  will be remembered until such time as "Command, Off" is used:
	if (splash.hwnd && !DllCall("IsWindow","PTR",splash.hwnd))
		splash.hwnd := 0
	if (show_it_only)
	{
		if (splash.hwnd && !DllCall("IsWindowVisible","PTR",splash.hwnd))
			DllCall("ShowWindow","PTR",splash.hwnd,"UInt",SW_SHOWNOACTIVATE) ;  See bottom of this function for comments on SW_SHOWNOACTIVATE.
		; else for simplicity, do nothing.
		return
	}

	if (!turn_off && splash.hwnd && (options_consist_of_bar_pos_only || aOptions="")) ;  The "modify existing window" mode is in effect.
	{
		;  If there is an existing window, just update its bar position and text.
		;  If not, do nothing since we don't have the original text of the window to recreate it.
		;  Since this is our thread's window, it shouldn't be necessary to use SendMessageTimeout()
		;  since the window cannot be hung since by definition our thread isn't hung.  Also, setting
		;  a text item from non-blank to blank is not supported so that elements can be omitted from an
		;  update command without changing the text that's in the window.  The script can specify %a_space%
		;  to explicitly make an element blank.
		if (bar_pos_has_been_set && splash.bar_pos != bar_pos) ;  Avoid unnecessary redrawing.
		{
			splash.bar_pos := bar_pos
			if (splash.hwnd_bar)
				DllCall("SendMessage","PTR",splash.hwnd_bar,"UInt", PBM_SETPOS,"PTR", bar_pos,"PTR", 0)
		}
		;  SendMessage() vs. SetWindowText() is used for controls so that tabs are expanded.
		;  For simplicity, the hwnd_text1 control is not expanded dynamically if it is currently of
		;  height zero.  The user can recreate the window if a different height is needed.
		if (aMainText!="" && splash.hwnd_text1)
			DllCall("SendMessage","PTR",splash.hwnd_text1,"UInt", WM_SETTEXT,"PTR" 0,"PTR", &aMainText)
		if (aSubText!="")
			DllCall("SendMessage","PTR",splash.hwnd_text2,"Uint", WM_SETTEXT,"PTR", 0,"PTR", &aSubText)
		if (aTitle!="")
			DllCall("SetWindowText","PTR",splash.hwnd,"Str", aTitle) ;  Use the simple method for parent window titles.
		return
	}

	;  Otherwise, destroy any existing window first:
	if (splash.hwnd)
		DllCall("DestroyWindow","PTR",splash.hwnd)
	if (splash.hfont1) ;  Destroy font only after destroying the window that uses it.
		DllCall("DeleteObject","PTR",splash.hfont1)
	if (splash.hfont2)
		DllCall("DeleteObject","PTR",splash.hfont2)
	if (splash.hbrush)
		DllCall("DeleteObject","PTR",splash.hbrush)
	if (splash.pic_bmp)
	{
		if (splash.pic_type == IMAGE_BITMAP)
			DllCall("DeleteObject","PTR",splash.pic_bmp)
		else
			DllCall("DestroyIcon","PTR",splash.pic_icon)
	}
	splash.Fill() ;  Set the above and all other fields to zero.

	if (turn_off){
    If splash.hwnd
		OnMessage(WM_ERASEBKGND,splash.hwnd,"")
		,OnMessage(WM_CTLCOLORSTATIC,splash.hwnd,"")
		,OnMessage(WM_SIZE,splash.hwnd,"")
		return
	}
	;  Otherwise, the window needs to be created or recreated.

	if (aTitle="") ;  Provide default title.
		aTitle := A_ScriptName ? A_ScriptName : ""

	;  Since there is often just one progress/splash window, and it defaults to always-on-top,
	;  it seems best to default owned to be true so that it doesn't get its own task bar button:
	owned := true          ;  Whether this window is owned by the main window.
	centered_main := true  ;  Whether the main text is centered.
	centered_sub := true   ;  Whether the sub text is centered.
	initially_hidden := false  ;  Whether the window should kept hidden (for later showing by the script).
	style := Struct("Int",[WS_DISABLED|WS_POPUP|WS_CAPTION])  ;  WS_CAPTION implies WS_BORDER
	exstyle := Struct("Int",[WS_EX_TOPMOST])
	xpos := Struct("Int",[COORD_UNSPECIFIED])
	ypos := Struct("Int",[COORD_UNSPECIFIED])
	range_min := 0, range_max := 0  ;  For progress bars.
	font_size1 := 0 ;  0 is the flag to "use default size".
	font_size2 := 0
	font_weight1 := FW_DONTCARE  ;  Flag later logic to use default.
	font_weight2 := FW_DONTCARE  ;  Flag later logic to use default.
	bar_color := CLR_DEFAULT
	splash.color_bk := CLR_DEFAULT
	splash.color_text := CLR_DEFAULT
	splash.height := COORD_UNSPECIFIED
	splash.width := SPLASH_DEFAULT_WIDTH
	splash.object_height := 20
	splash.object_width := COORD_UNSPECIFIED  ;  Currently only used for SplashImage, not Progress.
  splash.margin_x := 10
  splash.margin_y := 5
	cp:=(&aOptions)-2
	while (""!=cp_:=StrGet(cp:=cp+2,1)){
	;for (cp2, cp = options; cp!=""; ++cp)
		If (cp_="a"){  ;  Non-Always-on-top.  Synonymous with A0 in early versions.
			;  Decided against this enforcement.  In the enhancement mentioned below is ever done (unlikely),
			;  it seems that A1 can turn always-on-top on and A0 or A by itself can turn it off:
			; if (cp[1] == '0') ;  The zero is required to allow for future enhancement: modify attrib. of existing window.
			exstyle.1 &= ~WS_EX_TOPMOST
		} else if (cp_="b"){ ;  Borderless and/or Titleless
			style.1 &= ~WS_CAPTION
			if (StrGet(cp+2,1) = "1")
				style.1 |= WS_BORDER
			else if (StrGet(cp+2,1) = "2")
				style.1 |= WS_DLGFRAME
		} else if (cp_="c"){ ;  Colors
			if (""=SubStr(cp+2,1)) ;  Avoids out-of-bounds when the loop's own ++cp is done.
				continue
			cp+=2 ;  Always increment to omit the next char from consideration by the next loop iteration.
			If InStr("btw",cp_:=StrGet(cp,1)){
			; 'B': ;  Bar color.
			; 'T': ;  Text color.
			; 'W': ;  Window/Background color.
				color_str:=StrGet(cp+2,32)
				If (space_pos:=InStr(color_str," ")) ^ (tab_pos:=InStr(color_str,A_Tab))
					StrPut("",(&color_str)+(space_pos&&space_pos<tab_pos?space_pos:tab_pos?tab_pos:space_pos)*2)
				; else a color name can still be present if it's at the end of the string.
				color := ( !color_str || !InStr(".black.silver.gray.white.maroon.red.purple.fuchsia.green.lime.olive.yellow.navy.blue.teal.aqua.default.","." color_str ".") )
								? CLR_NONE : %aColorName%
				if (color == CLR_NONE) ;  A matching color name was not found, so assume it's in hex format.
				{
					if (StrLen(color_str) > 6)
						color_str:=SubStr(color_str,1,6)+0 ;  Shorten to exactly 6 chars, which happens if no space/tab delimiter is present.
					color := ((Color_str&255)<<16) + (((Color_str>>8)&255)<<8) + (Color_str>>16)
					;  if color_str does not contain something hex-numeric, black (0x00) will be assumed,
					;  which seems okay given how rare such a problem would be.
				}
				if (cp_="b"){
					bar_color := color
				} else if (cp_="t"){
					splash.color_text := color
				} else if (cp_="w"){
					splash.color_bk := color
					splash.hbrush := DllCall("CreateSolidBrush","Uint",color,"PTR") ;  Used for window & control backgrounds.
				}
				;  Skip over the color string to avoid interpreting hex digits or color names as option letters:
				cp += StrLen(color_str)+2
			} else {
			; default:
				centered_sub := (StrGet(cp,1) != "")
				centered_main := (StrGet(cp+2,1) != "0")
			}
		} else if (cp_="F"){
			if (""=SubStr(cp+2,1)) ;  Avoids out-of-bounds when the loop's own ++cp is done.
				continue
			cp+=2 ;  Always increment to omit the next char from consideration by the next loop iteration.
			If ("m"=cp_:=StrGet(cp,1)) {
				if ((font_size1 := _ttoi[cp + 2]) < 0)
					font_size1 := 0
			} else if (cp_="s"){
				if ((font_size2 := _ttoi[cp + 2]) < 0)
					font_size2 := 0
			}
		}else if (cp_="M"){ ;  Movable and (optionally) resizable.
			style.1 &= ~WS_DISABLED
			if (StrGet(cp + 2,1) = "1")
				style.1 |= WS_SIZEBOX
			if (StrGet(cp + 2,1) = "2")
				style.1 |= WS_SIZEBOX|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU
		} else if (cp_="p"){ ;  Starting position of progress bar [v1.0.25]
      bar_pos := _ttoi[cp + 2]
			bar_pos_has_been_set := true
		} else if (cp_="r"){ ;  Range of progress bar [v1.0.25]
			if (""=SubStr(cp+2,1)) ;  Ignore it because we don't want cp to ever poto the NULL terminator due to the loop's increment.
				continue
			range_min := _ttoi[cp + 2] ;  Increment cp to poit to range_min.
			if (cp < cp2 := cp+(InStr(StrGet(cp), "-")-1)*2)  ;  +1 to omit the min's minus sign, if it has one.
			{
				cp := cp2
				if (""=SubStr(cp+2,1)) ;  Ignore it because we don't want cp to ever poto the NULL terminator due to the loop's increment.
					continue
				range_max := _ttoi[cp + 2] ;  Increment cp to poit to range_max, which can be negative as in this example: R-100--50
			}
		} else if (cp_="t"){ ;  Give it a task bar button by making it a non-owned window.
			owned := false
		;  For options such as W, X and Y:
		;  Use atoi() vs. ATOI() to avoid interpreting something like 0x01B as hex when in fact
		;  the B was meant to be an option letter:
		} else if (cp_="w"){
			if (""=SubStr(cp+2,1)) ;  Avoids out-of-bounds when the loop's own ++cp is done.
				continue
			cp+= 2 ;  Always increment to omit the next char from consideration by the next loop iteration.
			if ("m"=cp_:=StrGet(cp,1)){
				if ((font_weight1 := _ttoi[cp + 2]) < 0)
					font_weight1 := 0
				break
			} else if (cp_="s"){
				if ((font_weight2:=_ttoi[cp + 2]) < 0)
					font_weight2 := 0
			} else
				splash.width := _ttoi[cp]
		} else if (cp_="h"){
			if (StrGet(cp,4)="Hide"){ ;  Hide vs. Hidden is debatable.
				initially_hidden := true
				cp+= 2*3 ;  +3 vs. +4 due to the loop's own ++cp.
			}	else ;  Allow any width/height to be specified so that the window can be "rolled up" to its title bar:
				splash.height := _ttoi[cp + 2]
		} else if (cp_="x"){
			xpos.1 := _ttoi[cp + 2]
		} else if (cp_="y"){
			ypos.1 := _ttoi[cp + 2]
		} else if (cp_="z"){
			if (""=SubStr(cp+2,1)) ;  Avoids out-of-bounds when the loop's own ++cp is done.
				continue
			cp+=2 ;  Always increment to omit the next char from consideration by the next loop iteration.
			If InStr("bh",cp_:=StrGet(cp,1)){
				splash.object_height := _ttoi[cp + 2] ;  Allow it to be zero or negative to omit the object.
			;} else if (cp_="w"){
				; else for Progress, don't allow width to be changed since a zero would omit the bar.
			} else if (cp_="x"){
				splash.margin_x := _ttoi[cp + 2]
			} else if (cp_="y"){
				splash.margin_y := _ttoi[cp + 2]
			}
		;  Otherwise: Ignore other characters, such as the digits that occur after the P/X/Y option letters.
		} ;  switch()
	} ;  for()

	hdc := DllCall("CreateDC","Str","DISPLAY","PTR", 0,"PTR", 0,"PTR", 0,"PTR")
	pixels_per_point_y := DllCall("GetDeviceCaps","PTR",hdc,"Uint",LOGPIXELSY)

	;  Get name and size of default font.
	hfont_default := DllCall("GetStockObject","UInt",DEFAULT_GUI_FONT,"PTR")
	hfont_old := DllCall("SelectObject","PTR",hdc,"PTR", hfont_default,"PTR")
	VarSetCapacity(default_font_name,65*A_IsUnicode)
	DllCall("GetTextFace","PTR",hdc,"Uint", 65 - 1,"PTR", &default_font_name)
	VarSetCapacity(default_font_name,-1)
	DllCall("GetTextMetrics","PTR",hdc,"PTR", tm[])
	default_gui_font_height := tm.tmHeight

	;  If both are zero or less, reset object height/width for maintainability and sizing later.
	;  However, if one of them is -1, the caller is asking for that dimension to be auto-calc'd
	;  to "keep aspect ratio" with the the other specified dimension:
	if (   splash.object_height < 1 && splash.object_height != COORD_UNSPECIFIED
		&& splash.object_width < 1 && splash.object_width != COORD_UNSPECIFIED
		|| !splash.object_height || !splash.object_width   )
		splash.object_height := splash.object_width := 0

	;  If width is still unspecified -- which should only happen if it's a SplashImage window with
	;  no image, or there was a problem getting the image above -- set it to be the default.
	if (splash.width == COORD_UNSPECIFIED)
		splash.width := SPLASH_DEFAULT_WIDTH
	;  Similarly, object_height is set to zero if the object is not present:
	if (splash.object_height == COORD_UNSPECIFIED)
		splash.object_height := 0

	;  Lay out client area.  If height is COORD_UNSPECIFIED, use a temp value for now until
	;  it can be later determined.
	client_rect.Fill(), draw_rect.Fill()
	DllCall("SetRect","PTR",client_rect[],"Uint", 0,"Uint", 0,"Uint", splash.width,"Uint", splash.height == COORD_UNSPECIFIED ? 500 : splash.height)

	;  Create fonts based on specified posizes.  A zero value for font_size1 & 2 are correctly handled
	;  by CreateFont():
	if (aMainText!="")
	{
		;  If a zero size is specified, it should use the default size.  But the default brought about
		;  by passing a zero here is not what the system uses as a default, so instead use a font size
		;  that is 25% larger than the default size (since the default size itself is used for aSubtext).
		;  On a typical system, the default GUI font's posize is 8, so this will make it 10 by default.
		;  Also, it appears that changing the system's font size in Control Panel -> Display -> Appearance
		;  does not affect the reported default font size.  Thus, the default is probably 8/9 for most/all
		;  XP systems and probably other OSes as well.
		;  By specifying PROOF_QUALITY the nearest matching font size should be chosen, which should avoid
		;  any scaling artifacts that might be caused if default_gui_font_height is not 8.
		if (   !(splash.hfont1 := DllCall("CreateFont","Uint",font_size1 ? -DllCall("MulDiv","Int",font_size1,"Int", pixels_per_point_y,"Int", 72) : (1.25 * default_gui_font_height)
			,"Uint", 0,"Uint", 0,"Uint", 0,"Uint", font_weight1 ? font_weight1 : FW_SEMIBOLD,"Uint", 0,"Uint", 0,"Uint", 0,"Uint", DEFAULT_CHARSET,"Uint", OUT_TT_PRECIS
			,"Uint", CLIP_DEFAULT_PRECIS,"Uint", PROOF_QUALITY,"Uint", FF_DONTCARE,"Str", aFontName!="" ? aFontName : default_font_name,"PTR"))   )
			;  Call it again with default font in case above failed due to non-existent aFontName.
			;  Update: I don't think this actually does any good, at least on XP, because it appears
			;  that CreateFont() does not fail merely due to a non-existent typeface.  But it is kept
			;  in case it ever fails for other reasons:
			splash.hfont1 := DllCall("CreateFont","Uint",font_size1 ? -DllCall("MulDiv","Int",font_size1,"Int", pixels_per_point_y,"Int", 72) : (1.25 * default_gui_font_height)
				,"Uint","Uint", 0,"Uint", 0,"Uint", 0,"Uint", font_weight1 ? font_weight1 : FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS
				,"Uint", CLIP_DEFAULT_PRECIS,"Uint", PROOF_QUALITY,"Uint", FF_DONTCARE,"Str", default_font_name,"PTR")
		;  To avoid showing a runtime error, fall back to the default font if other font wasn't created:
		DllCall("SelectObject","PTR",hdc,"PTR", splash.hfont1 ? splash.hfont1 : hfont_default)
		;  Calc height of text by taking into account font size, number of lines, and space between lines:
		draw_rect[] := client_rect
		draw_rect.left += splash.margin_x
		draw_rect.right -= splash.margin_x
		splash.text1_height := DllCall("DrawText","PTR",hdc,"Str", aMainText,"Uint", -1,"PTR", draw_rect[],"Uint", DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS)
	}
	;  else leave the above fields set to the zero defaults.

	if (font_size2 || font_weight2 || aFontName)
		if (   !(splash.hfont2 := DllCall("CreateFont","Uint",-DllCall("MulDiv","Int",font_size2,"Int", pixels_per_point_y,"Int", 72),"Uint", 0,"Uint", 0,"Uint", 0
			,"Uint", font_weight2,"Uint", 0,"Uint", 0,"Uint", 0,"Uint", DEFAULT_CHARSET,"Uint", OUT_TT_PRECIS,"Uint", CLIP_DEFAULT_PRECIS
			,"Uint", PROOF_QUALITY,"Uint", FF_DONTCARE,"Str", aFontName!="" ? aFontName : default_font_name,"PTR"))   )
			;  Call it again with default font in case above failed due to non-existent aFontName.
			;  Update: I don't think this actually does any good, at least on XP, because it appears
			;  that CreateFont() does not fail merely due to a non-existent typeface.  But it is kept
			;  in case it ever fails for other reasons:
			if (font_size2 || font_weight2)
				splash.hfont2 := DllCall("CreateFont","Uint",-DllCall("MulDiv","Int",font_size2,"Int", pixels_per_point_y,"Int", 72),"Uint", 0,"Uint", 0,"Uint", 0
					,"Uint", font_weight2,"Uint", 0,"Uint", 0,"Uint", 0,"Uint", DEFAULT_CHARSET,"Uint", OUT_TT_PRECIS,"Uint", CLIP_DEFAULT_PRECIS
					,"Uint", PROOF_QUALITY,"Uint", FF_DONTCARE,"Str", default_font_name,"PTR")
	; else leave it NULL so that hfont_default will be used.

	;  The font(s) will be deleted the next time this window is destroyed or recreated,
	;  or by the g_script destructor.

	bar_y := splash.margin_y + (splash.text1_height ? (splash.text1_height + splash.margin_y) : 0)
	sub_y := bar_y + splash.object_height + (splash.object_height ? splash.margin_y : 0)  ;  Calculate the Y position of each control in the window.

	if (splash.height == COORD_UNSPECIFIED)
	{
		;  Since the window height was not specified, determine what it should be based on the height
		;  of all the controls in the window:
		if (aSubText!="")
		{
			DllCall("SelectObject","PTR",hdc,"PTR", splash.hfont2 ? splash.hfont2 : hfont_default)
			;  Calc height of text by taking into account font size, number of lines, and space between lines:
			;  Reset unconditionally because the previous DrawText() sometimes alters the rect:
			draw_rect[] := client_rect
			draw_rect.left += splash.margin_x
			draw_rect.right -= splash.margin_x
			subtext_height := DllCall("DrawText","PTR",hdc,"Str", aSubText,"Uint", -1,"PTR", draw_rect[],"Uint", DT_CALCRECT | DT_WORDBREAK)
		}
		else
			subtext_height := 0
		;  For the below: sub_y was previously calc'd to be the top of the subtext control.
		;  Also, splash.margin_y is added because the text looks a little better if the window
		;  doesn't end immediately beneath it:
		splash.height := subtext_height + sub_y + splash.margin_y
		client_rect.bottom := splash.height
	}

	DllCall("SelectObject","PTR",hdc,"PTR", hfont_old) ;  Necessary to avoid memory leak.
	if !DllCall("DeleteDC","PTR",hdc){
		ErrorLevel := -1
		return  ;  Force a failure to detect bugs such as hdc still having a created handle inside.
	}
	;  Based on the client area determined above, expand the main_rect to include title bar, borders, etc.
	;  If the window has a border or caption this also changes top & left *slightly* from zero.
	main_rect[] := client_rect
	DllCall("AdjustWindowRectEx","PTR",main_rect[],"Uint", style.1,"Uint", FALSE,"Uint", exstyle.1)
	main_width := main_rect.right - main_rect.left  ;  main.left might be slightly less than zero.
	main_height := main_rect.bottom - main_rect.top ;  main.top might be slightly less than zero.

	work_rect.Fill()
	DllCall("SystemParametersInfo","Uint",SPI_GETWORKAREA,"Uint", 0,"PTR", work_rect[],"Uint", 0)  ;  Get desktop rect excluding task bar.
	work_width := work_rect.right - work_rect.left  ;  Note that "left" won't be zero if task bar is on left!
	work_height := work_rect.bottom - work_rect.top  ;  Note that "top" won't be zero if task bar is on top!

	;  Seems best (and easier) to unconditionally restrict window size to the size of the desktop,
	;  since most users would probably want that.  This can be overridden by using WinMove afterward.
	if (main_width > work_width)
		main_width := work_width
	if (main_height > work_height)
		main_height := work_height

	;  Centering doesn't currently handle multi-monitor systems explicitly, since those calculations
	;  require API functions that don't exist in Win95/NT (and thus would have to be loaded
	;  dynamically to allow the program to launch).  Therefore, windows will likely wind up
	;  being centered across the total dimensions of all monitors, which usually results in
	;  half being on one monitor and half in the other.  This doesn't seem too terrible and
	;  might even be what the user wants in some cases (i.e. for really big windows).
	;  See comments above for why work_rect.left and top are added in (they aren't always zero).
	if (xpos.1 = COORD_UNSPECIFIED)
		xpos.1 := work_rect.left + ((work_width - main_width) // 2)  ;  Don't use splash.width.
	if (ypos.1 = COORD_UNSPECIFIED)
		ypos.1 := work_rect.top + ((work_height - main_height) // 2)  ;  Don't use splash.width.

	;  CREATE Main Splash Window
	;  It seems best to make this an unowned window for two reasons:
	;  1) It will get its own task bar icon then, which is usually desirable for cases where
	;     there are several progress/splash windows or the window is monitoring something.
	;  2) The progress/splash window won't prevent the main window from being used (owned windows
	;     prevent their owners from ever becoming active).
	;  However, it seems likely that some users would want the above to be configurable,
	;  so now there is an option to change this behavior.
	dialog_owner := 0 ;THREAD_DIALOG_OWNER  ;  Resolve macro only once to reduce code size.
	splash.hwnd := DllCall("CreateWindowEx","UInt", exstyle.1 ,"Str", "AutoHotkeyGUI","Str", aTitle ,"Uint", style.1,"Uint", xpos.1 ,"Uint", ypos.1
								;  v1.0.35.01: For flexibility, allow these windows to be owned by GUIs via +OwnDialogs.
								,"Uint", main_width ,"Uint", main_height ,"PTR", owned ? (dialog_owner ? dialog_owner : A_ScriptHwnd) : 0
								,"PTR", 0,"PTR", A_ModuleHandle, "PTR",0,"PTR")
	OnMessage(WM_ERASEBKGND,splash.hwnd,"SplashImage_OnMessage")
	,OnMessage(WM_CTLCOLORSTATIC,splash.hwnd,"SplashImage_OnMessage")
	,OnMessage(WM_SIZE,splash.hwnd,"SplashImage_OnMessage")
	if !(splash.hwnd){
		ErrorLevel:=-1
		return   ;  No error msg since so rare.
	}
	
	if ((style.1 & WS_SYSMENU) || !owned)
	{
		;  Setting the small icon puts it in the upper left corner of the dialog window.
		;  Setting the big icon makes the dialog show up correctly in the Alt-Tab menu (but big seems to
		;  have no effect unless the window is unowned, i.e. it has a button on the task bar).
		
		;  L17: Use separate big/small icons for best results.
		if (g_script.mCustomIcon)
		{
			big_icon := g_script.mCustomIcon
			small_icon := g_script.mCustomIconSmall ;  Should always be non-NULL when mCustomIcon is non-NULL.
		}
		else
			big_icon := small_icon := DllCall("LoadImage","PTR",A_ModuleHandle,"PTR", IDI_MAIN & 0xFFFF,"UInt", IMAGE_ICON,"Int", 0,"Int", 0,"UInt", LR_SHARED,"PTR") ;  Use LR_SHARED to conserve memory (since the main icon is loaded for so many purposes).

		if (style.1 & WS_SYSMENU)
			DllCall("SendMessage","PTR",splash.hwnd,"Uint", WM_SETICON,"PTR", ICON_SMALL,"PTR", small_icon)
		if (!owned)
			DllCall("SendMessage","PTR",splash.hwnd,"Uint", WM_SETICON,"PTR", ICON_BIG,"PTR", big_icon)
	}

	;  Update client rect in case it was resized due to being too large (above) or in case the OS
	;  auto-resized it for some reason.  These updated values are also used by SPLASH_CALC_CTRL_WIDTH
	;  to position the static text controls so that text will be centered properly:
	DllCall("GetClientRect","PTR",splash.hwnd,"PTR", client_rect[])
	splash.height := client_rect.bottom
	splash.width := client_rect.right
	control_width := client_rect.right - (splash.margin_x * 2)

	;  CREATE Main label
	if (aMainText!="")
	{
		splash.hwnd_text1 := DllCall("CreateWindowEx","UInt",0,"Str", "static","Str", aMainText
			,"Uint", WS_CHILD|WS_VISIBLE|SS_NOPREFIX|(centered_main ? SS_CENTER : SS_LEFT)
			,"Uint", splash.margin_x,"Uint", splash.margin_y,"Uint", control_width,"Uint", splash.text1_height,"PTR", splash.hwnd,"PTR", 0,"PTR", A_ModuleHandle,"PTR", 0,"PTR")
		DllCall("SendMessage","PTR",splash.hwnd_text1,"UInt", WM_SETFONT,"PTR", (splash.hfont1 ? splash.hfont1 : hfont_default),"PTR", ((1 & 0xffff) | (0 & 0xffff) << 16) & 0xFFFF)
	}

	if (splash.object_height > 0) ;  Progress window
	{
		;  CREATE Progress control (always starts off at its default position as determined by OS/common controls):
		if (splash.hwnd_bar := DllCall("CreateWindowEx","UInt",WS_EX_CLIENTEDGE,"Str", PROGRESS_CLASS,"Str", "","Uint", WS_CHILD|WS_VISIBLE|PBS_SMOOTH
			,"Uint", splash.margin_x,"Uint", bar_y,"Uint", control_width,"Uint", splash.object_height,"PTR", splash.hwnd,"PTR", 0,"PTR", 0,"PTR", 0,"PTR"))
		{
			if (range_min || range_max) ;  i.e. if both are zero, leave it at the default range, which is 0-100.
			{
				if (range_min > -1 && range_min < 0x10000 && range_max > -1 && range_max < 0x10000)
					;  Since the values fall within the bounds for Win95/NT to support, use the old method
					;  in case Win95/NT lacks MSIE 3.0:
					DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_SETRANGE,"PTR", 0,"PTR", (range_min & 0xffff) | (range_max & 0xffff) << 16 & 0xFFFF)
				else
					DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_SETRANGE32,"PTR","PTR", range_min,"PTR", range_max)
			}


			if (bar_color != CLR_DEFAULT)
			{
				;  Remove visual styles so that specified color will be obeyed:
				DllCall("MySetWindowTheme","PTR",splash.hwnd_bar,"WStr", "","WStr", "")
				DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_SETBARCOLOR,"PTR", 0,"PTR", bar_color) ;  Set color.
			}
			if (splash.color_bk != CLR_DEFAULT)
				DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_SETBKCOLOR,"PTR", 0,"PTR", splash.color_bk) ;  Set color.
			if (bar_pos_has_been_set) ;  Note that the window is not yet visible at this stage.
				;  This happens when the window doesn't exist and a command such as the following is given:
				;  Progress, 50 [, ...].  As of v1.0.25, it also happens via the new 'P' option letter:
				DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_SETPOS,"PTR", bar_pos,"PTR", 0)
			else ;  Ask the control its starting/default position in case a custom range is in effect.
				bar_pos := DllCall("SendMessage","PTR",splash.hwnd_bar,"Uint", PBM_GETPOS,"PTR", 0,"PTR", 0,"PTR")
			splash.bar_pos := bar_pos ;  Save the current position to avoid redraws when future positions are identical to current.
		}
	}

	;  CREATE Sub label
	if (splash.hwnd_text2 := DllCall("CreateWindowEx","UInt",0,"Str", "static","Str", aSubText
		,"Uint", WS_CHILD|WS_VISIBLE|SS_NOPREFIX|(centered_sub ? SS_CENTER : SS_LEFT)
		,"Uint", splash.margin_x,"Uint", sub_y,"Uint", control_width,"Uint", (client_rect.bottom - client_rect.top) - sub_y,"PTR", splash.hwnd,"PTR", 0,"PTR", A_ModuleHandle, "PTR", 0,"PTR"))
		DllCall("SendMessage","PTR",splash.hwnd_text2,"Uint", WM_SETFONT,"PTR",(splash.hfont2 ? splash.hfont2 : hfont_default),"PTR", ((1 & 0xffff) | (0 & 0xffff) << 16) & 0xFFFF)
	;  Show it without activating it.  Even with options that allow the window to be activated (such
	;  as movable), it seems best to do this to prevent changing the current foreground window, which
	;  is usually desirable for progress/splash windows since they should be seen but not be disruptive:
	if (!initially_hidden)
		DllCall("ShowWindow","PTR",splash.hwnd,"UInt", SW_SHOWNOACTIVATE)
	return
}