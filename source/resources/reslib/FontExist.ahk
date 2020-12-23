FontExist(aHdc,aTypeface){
	static DEFAULT_CHARSET:=1,LOGFONT:="LONG lfHeight;LONG lfWidth;LONG lfEscapement;LONG lfOrientation;LONG lfWeight;BYTE lfItalic;BYTE lfUnderline;BYTE lfStrikeOut;BYTE lfCharSet;BYTE lfOutPrecision;BYTE lfClipPrecision;BYTE lfQuality;BYTE lfPitchAndFamily;TCHAR lfFaceName[32]"
        ,lf:=Struct(LOGFONT) ;LF_FACESIZE:=32
        ,FontEnumProc:=CallbackCreate("FontEnumProc")
	lf.lfCharSet := DEFAULT_CHARSET  ; Enumerate all char sets.
	lf.lfPitchAndFamily := 0  ; Must be zero.
	StrPut(aTypeface,lf.lfFaceName[""])
	DllCall("EnumFontFamiliesEx","PTR",aHdc,"PTR", lf[],"PTR", FontEnumProc,"CHAR*", font_exists:=0,"Int", 0)
	return font_exists
}
FontEnumProc(lpelfe, lpntme, FontType, lParam){
	NumPut("Char",1,lParam) ; Indicate to the caller that the font exists.
	return 0  ; Stop the enumeration after the first, since even one match means the font exists.
}