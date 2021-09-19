EnvUpdate(){
	static HWND_BROADCAST:=0xFFFF,WM_SETTINGCHANGE:=0x001A,Environment:="Environment"
	SendMessage WM_SETTINGCHANGE, 0, StrPtr(Environment),, "ahk_id " HWND_BROADCAST
}