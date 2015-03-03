TrayTip(aTitle:="",aText:="",aTimeout:=0, aOptions:=0){ ; moved aText and aTimeout to front becaus aTitle may be omited and text and timeout are mostly used
	static osversion,temp1:=VarSetCapacity(osversion,4),temp2:=NumPut(DllCall("GetVersion"),osversion),os:=Struct("byte major,byte minor",&osversion)
				,nic := Struct("DWORD cbSize,HWND hWnd,UINT uID,UINT uFlags,UINT uCallbackMessage,HICON hIcon,TCHAR szTip[128],DWORD dwState"
				. ",DWORD dwStateMask,TCHAR szInfo[256],{UINT uTimeout,UINT uVersion},TCHAR szInfoTitle[64],DWORD dwInfoFlags,Data1,ushort Data2,ushort Data3,byte Data4[8]" (os.major>5?"HICON hBalloonIcon":""))
				,AHK_NOTIFYICON:=1028,NIF_INFO:=0x10,NIM_MODIFY:=0x1,Shell:=DllCall("LoadLibrary","Str","shell32.dll","PTR")
	if (os.major < 5) ; Older OSes do not support it, so do nothing.
		return
	nic.Fill() 								; set structure empty
	nic.cbSize := sizeof(nic)
	nic.uID := AHK_NOTIFYICON  ; This must match our tray icon's uID or Shell_NotifyIcon() will return failure.
	nic.hWnd := A_ScriptHwnd
	nic.uFlags := NIF_INFO
	nic.uTimeout := aTimeout * 1000
	nic.dwInfoFlags := aOptions + 0
	StrPut(aTitle,nic.szInfoTitle[""],nic.CountOf("szInfoTitle"))
	StrPut(aText,nic.szInfo[""],nic.CountOf("szInfo"))
	DllCall("shell32.dll\Shell_NotifyIconW","UInt",NIM_MODIFY,"PTR",nic[])
	return 1
}
