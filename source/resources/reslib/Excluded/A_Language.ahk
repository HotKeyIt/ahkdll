A_Language(b:=0,n:=0){
static d:=RegRead("HKLM\SYSTEM\CurrentControlSet\Control\Nls\Language", "InstallLanguage"),l:=StrLen(d)
Critical
return b?StrPut(d,b):n?l:d
}