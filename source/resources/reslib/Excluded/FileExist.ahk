FileExist(f){
	static FILETIME:="DWORD dwLowDateTime;DWORD dwHighDateTime"
				,wfd:=Struct("DWORD dwFileAttributes;FileExist(FILETIME) ftCreationTime;FileExist(FILETIME) ftLastAccessTime;FileExist(FILETIME) ftLastWriteTime;DWORD nFileSizeHigh;DWORD nFileSizeLow;DWORD dwReserved0;DWORD dwReserved1;TCHAR cFileName[260];TCHAR cAlternateFileName[14]") ;WIN32_FIND_DATA
				,READONLY:=1,ARCHIVE:=32,SYSTEM:=4,HIDDEN:=2,NORMAL:=128,DIRECTORY:=16,OFFLINE:=4096,COMPRESSED:=2048,TEMPORARY:=256
	if (InStr(f,"*") || InStr(f,"?")){
		if -1=hFile := DllCall("FindFirstFile","Str",f,"PTR", wfd[])
			return ""
		a:=wfd.dwFileAttributes,DllCall("FindClose","PTR",hFile)
	} else a:=DllCall("GetFileAttributes","Str",f,"UInt") ;!= 0xffffffff
  if a!=0xffffffff
    return (a&ReadOnly?"R":"") (a&ARCHIVE?"A":"") (a&SYSTEM?"S":"") (a&HIDDEN?"H":"") (a&NORMAL?"N":"") (a&DIRECTORY?"D":"") (a&OFFLINE?"O":"") (a&COMPRESSED?"C":"") (a&TEMPORARY?"T":"")
}