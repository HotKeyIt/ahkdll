FileGetVersion(aFilespec){

  static VS_FIXEDFILEINFO := "
  (
    DWORD   dwSignature;            // e.g. 0xfeef04bd
    DWORD   dwStrucVersion;         // e.g. 0x00000042 = "0.42" 
    DWORD   dwFileVersionMS;        // e.g. 0x00030075 = "3.75" 
    DWORD   dwFileVersionLS;        // e.g. 0x00000031 = "0.31" 
    DWORD   dwProductVersionMS;     // e.g. 0x00030010 = "3.10" 
    DWORD   dwProductVersionLS;     // e.g. 0x00000031 = "0.31" 
    DWORD   dwFileFlagsMask;        // = 0x3F for version "0.42" 
    DWORD   dwFileFlags;            // e.g. VFF_DEBUG | VFF_PRERELEASE
    DWORD   dwFileOS;               // e.g. VOS_DOS_WINDOWS16
    DWORD   dwFileType;             // e.g. VFT_DRIVER
    DWORD   dwFileSubtype;          // e.g. VFT2_DRV_KEYBOARD
    DWORD   dwFileDateMS;           // e.g. 0
    DWORD   dwFileDateLS;           // e.g. 0
  )"
  , pFFI:=Struct("FileGetVersion(VS_FIXEDFILEINFO)*")
	if (aFilespec=""){
    ErrorLevel := 1
		return ;SetErrorsOrThrow(true, ERROR_INVALID_PARAMETER) ; Error out, since this is probably not what the user intended.
  }
  
	if !(dwSize := DllCall("version\GetFileVersionInfoSizeW","STR",aFilespec,"Int*", dwUnused)){  ; No documented limit on how large it can be, so don't use _alloca().
    ErrorLevel := 1
    throw 1 ;SetErrorsOrThrow(true)
  }
  VarSetCapacity(pInfo,dwSize,0)
  
	; Read the version resource
	if (!(a:=DllCall("version\GetFileVersionInfoW","Str",aFilespec,"Int", 0,"Int", dwSize,"PTR", &pInfo))
      ; Locate the fixed information
      || !DllCall("version\VerQueryValueW","PTR",&pInfo,"Str", "\","PTR", pFFI[],"PTR*", uSize)){
    ErrorLevel := 1
		throw 0 ;SetErrorLevelOrThrow()
	}

	; extract the fields you want from pFFI
	iFileMS := pFFI.dwFileVersionMS
	iFileLS := pFFI.dwFileVersionLS
	
  ErrorLevel := 0 ; Indicate success.
	return (iFileMS >> 16) "." (iFileMS & 0xFFFF) "." (iFileLS >> 16) "." (iFileLS & 0xFFFF)
}
