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
FontEnumProc(lpelfe, lpntme, FontType, lParam)
{ ; LF_FULLFACESIZE:=64,LF_FACESIZE:=32
  ;~ static elfe:=Struct("FontExist(LOGFONT) elfLogFont;TCHAR elfFullName[64];TCHAR elfStyle[32];TCHAR elfScript[32]") ;ENUMLOGFONTEX 
        ;~ ,NEWTEXTMETRIC:="LONG tmHeight;LONG tmAscent;LONG tmDescent;LONG tmInternalLeading;LONG tmExternalLeading;LONG tmAveCharWidth;LONG tmMaxCharWidth;LONG tmWeight;LONG tmOverhang;LONG tmDigitizedAspectX;LONG tmDigitizedAspectY;TCHAR tmFirstChar;TCHAR tmLastChar;TCHAR tmDefaultChar;TCHAR tmBreakChar;BYTE tmItalic;BYTE tmUnderlined;BYTE tmStruckOut;BYTE tmPitchAndFamily;BYTE tmCharSet;DWORD ntmFlags;UINT ntmSizeEM;UINT ntmCellHeight;UINT ntmAvgWidth"
        ;~ ,FONTSIGNATURE:="DWORD fsUsb[4];DWORD fsCsb[2]"
        ;~ ,ntme:=Struct("FontEnumProc(NEWTEXTMETRIC) ntmTm;FontEnumProc(FONTSIGNATURE) ntmFontSig")
  ;~ elfe[]:=lpelfe,ntme[]:=lpntme
	NumPut(1,lParam,"Char") ; Indicate to the caller that the font exists.
	return 0  ; Stop the enumeration after the first, since even one match means the font exists.
}