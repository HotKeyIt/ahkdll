Download(aURL, aFilespec){
	; Check that we have IE3 and access to wininet.dll
	static hinstLib := DllCall("LoadLibrary","Str","wininet","PTR"),WINAPI_SUFFIX:="W",IRF_NO_WAIT:=8
        ,INTERNET_BUFFERSA:="
        (
          DWORD dwStructSize;                 // used for API versioning. Set to sizeof(INTERNET_BUFFERS)
          Download(INTERNET_BUFFERSA) *Next; // chain of buffers
          LPCSTR   lpcszHeader;               // pointer to headers (may be NULL)
          DWORD dwHeadersLength;              // length of headers if not NULL
          DWORD dwHeadersTotal;               // size of headers if not enough buffer
          LPVOID lpvBuffer;                   // pointer to data buffer (may be NULL)
          DWORD dwBufferLength;               // length of data buffer if not NULL
          DWORD dwBufferTotal;                // total size of chunk, or content-length if not chunked
          DWORD dwOffsetLow;                  // used for read-ranges (only used in HttpSendRequest2)
          DWORD dwOffsetHigh
        )"
        ,INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY:=4
        ,INTERNET_FLAG_RELOAD:=2147483648,INTERNET_FLAG_NO_CACHE_WRITE:=67108864
        ,buffers:=Struct("Download(INTERNET_BUFFERSA)"),PM_NOREMOVE:=0,g_script:=ScriptStruct(),g:=GlobalStruct()
        ; Get the address of all the functions we require.  It's done this way in case the system
        ; lacks MSIE v3.0+, in which case the app would probably refuse to launch at all:
        ,lpfnInternetOpen := DllCall("GetProcAddress","PTR",hinstLib,"AStr", "InternetOpen" WINAPI_SUFFIX,"PTR")
        ,lpfnInternetOpenUrl := DllCall("GetProcAddress","PTR",hinstLib,"AStr", "InternetOpenUrl" WINAPI_SUFFIX,"PTR")
        ,lpfnInternetCloseHandle := DllCall("GetProcAddress","PTR",hinstLib,"AStr", "InternetCloseHandle","PTR")
        ,lpfnInternetReadFileEx := DllCall("GetProcAddress","PTR",hinstLib,"AStr", "InternetReadFileExA","PTR") ; InternetReadFileExW() appears unimplemented prior to Windows 7, so always use InternetReadFileExA().
        ,lpfnInternetReadFile := DllCall("GetProcAddress","PTR",hinstLib,"AStr", "InternetReadFile","PTR") ; Called unconditionally to reduce code size and because the time required is likely insignificant compared to network latency.
        ,init:= !(lpfnInternetOpen && lpfnInternetOpenUrl && lpfnInternetCloseHandle && lpfnInternetReadFileEx && lpfnInternetReadFile) ? DllCall("FreeLibrary","PTR",hinstLib) : 1
	if (!init){
    ErrorLevel:=1
		return
  }
	;~ typedef HINTERNET (WINAPI *MyInternetOpen)(LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD dwFlags)
	;~ typedef HINTERNET (WINAPI *MyInternetOpenUrl)(HINTERNET hInternet, LPCTSTR, LPCTSTR, DWORD, DWORD, LPDWORD)
	;~ typedef BOOL (WINAPI *MyInternetCloseHandle)(HINTERNET)
	;~ typedef BOOL (WINAPI *MyInternetReadFileEx)(HINTERNET, LPINTERNET_BUFFERSA, DWORD, DWORD)
	;~ typedef BOOL (WINAPI *MyInternetReadFile)(HINTERNET, LPVOID, DWORD, LPDWORD)

	; v1.0.44.07: Set default to INTERNET_FLAG_RELOAD vs. 0 because the vast majority of usages would want
	; the file to be retrieved directly rather than from the cache.
	; v1.0.46.04: Added more no-cache flags because otherwise, it definitely falls back to the cache if
	; the remote server doesn't repond (and perhaps other errors), which defeats the ability to use the
	; Download command for uptime/server monitoring.  Also, in spite of what MSDN says, it seems nearly
	; certain based on other sources that more than one flag is supported.  Someone also mentioned that
	; INTERNET_FLAG_CACHE_IF_NET_FAIL is related to this, but there's no way to specify it in these
	; particular calls, and it's the opposite of the desired behavior anyway so it seems impossible to
	; turn it off explicitly.
	flags_for_open_url := INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE
	aURL := ltrim(aURL)
	if (InStr(aURL,"*")=1) ; v1.0.44.07: Provide an option to override flags_for_open_url.
	{
    LoopParse,%aURL%,%A_Space%`t
    {
      flags_for_open_url := SubStr(A_LoopField,2)+0
      break
    }
    aURL:=SubStr(aURL,StrLen(flags_for_open_url)+2)
	}

	; Open the internet session. v1.0.45.03: Provide a non-NULL user-agent because  some servers reject
	; requests that lack a user-agent.  Furthermore, it's more professional to have one, in which case it
	; should probably be kept as simple and unchanging as possible.  Using something like the script's name
	; as the user agent (even if documented) seems like a bad idea because it might contain personal/sensitive info.
	if !(hInet := DllCall(lpfnInternetOpen,"Str","AutoHotkey","Int", INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY,"PTR", 0,"PTR", 0,"Int", 0,"PTR")){
    ErrorLevel:=1
		return
	}

	; Open the required URL
	if !(hFile := DllCall(lpfnInternetOpenUrl,"PTR",hInet,"Str", aURL,"PTR", 0,"Int", 0,"Int", flags_for_open_url,"PTR", 0,"PTR")){
    DllCall(lpfnInternetCloseHandle,"PTR",hInet)
		ErrorLevel:=1
		return
	}

	; Open our output file
	if !(fptr := FileOpen(aFilespec, "w")){	; Open in binary write/destroy mode
		DllCall(lpfnInternetCloseHandle,"PTR",hFile)
		DllCall(lpfnInternetCloseHandle,"PTR",hInet)
		ErrorLevel:=1
		return
	}

	VarSetCapacity(bufData,1024 * 1) ; v1.0.44.11: Reduced from 8 KB to alleviate GUI window lag during Download.  Testing shows this reduction doesn't affect performance on high-speed downloads (in fact, downloads are slightly faster I tested two sites, one at 184 KB/s and the other at 380 KB/s).  It might affect slow downloads, but that seems less likely so wasn't tested.
	buffers.Fill()
	buffers.dwStructSize := sizeof(INTERNET_BUFFERSA)
	buffers.lpvBuffer := &bufData
	buffers.dwBufferLength := 1024 ;* 1

	;~ LONG_OPERATION_INIT

	; Read the file.  I don't think synchronous transfers typically generate the pseudo-error
	; ERROR_IO_PENDING, so that is not checked here.  That's probably just for async transfers.
	; IRF_NO_WAIT is used to avoid requiring the call to block until the buffer is full.  By
	; having it return the moment there is any data in the buffer, the program is made more
	; responsive, especially when the download is very slow and/or one of the hooks is installed:
	result:=0
	if (InStr(aURL,"H")=1){
		while (result := DllCall(lpfnInternetReadFileEx,"PTR",hFile,"PTR", buffers[],"Int", IRF_NO_WAIT,"Int", 0)) ; Assign
		{
			if (!buffers.dwBufferLength) ; Transfer is complete.
				break
			;~ LONG_OPERATION_UPDATE  ; Done in between the net-read and the file-write to improve avg. responsiveness.
      tick_now := A_TickCount
  		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
  			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", PM_NOREMOVE)
  				Sleep -1
  			tick_now := A_TickCount
  			g_script.mLastPeekTime := tick_now
  		}
      
			fptr.RawWrite(&bufData, buffers.dwBufferLength)
			buffers.dwBufferLength := 1024  ; Reset buffer capacity for next iteration.
		}
	}
	else ; v1.0.48.04: This section adds support for FTP and perhaps Gopher by using InternetReadFile() instead of InternetReadFileEx().
	{
		while (result := DllCall(lpfnInternetReadFile,"PTR",hFile,"PTR", &bufData,"Int", 1024,"Int*", number_of_bytes_read))
		{
			if (!number_of_bytes_read)
				break
			;~ LONG_OPERATION_UPDATE
      tick_now := A_TickCount
  		if (A_TickCount - g_script.mLastPeekTime > g.PeekFrequency){
  			if DllCall("PeekMessage","PTR",msg[],"PTR", 0,"UInt", 0,"UInt", 0,"UInt", PM_NOREMOVE)
  				Sleep -1
  			tick_now := A_TickCount
  			g_script.mLastPeekTime := tick_now
  		}
      
			fptr.RawWrite(&bufData, number_of_bytes_read)
		}
	}
	; Close internet session:
	DllCall(lpfnInternetCloseHandle,"PTR",hFile)
	DllCall(lpfnInternetCloseHandle,"PTR",hInet)
	;FreeLibrary(hinstLib) ; Only after the above.
	; Close output file:
	fptr.Close()
  
	if (!result) ; An error occurred during the transfer.
		FileDelete(aFilespec)  ; Delete damaged/incomplete file.
	ErrorLevel:=!result
  return ;SetErrorLevelOrThrowBool(!result)
}