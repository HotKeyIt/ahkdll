Shutdown(nFlag){ ; Shutdown or logoff the system. Returns false if the function could not get the rights to shutdown.
  static EWX_LOGOFF:=0,EWX_SHUTDOWN:=0x00000001,EWX_REBOOT:=0x00000002,EWX_FORCE:=0x00000004,EWX_POWEROFF:=0x00000008,SE_PRIVILEGE_ENABLED:=2
        ,TOKEN_ADJUST_PRIVILEGES:=32,TOKEN_QUERY:=8,LUID:="DWORD LowPart;LONG  HighPart",LUID_AND_ATTRIBUTES :="ShutDown(LUID) Luid;DWORD Attributes"
        ,TOKEN_PRIVILEGES:="DWORD PrivilegeCount;ShutDown(LUID_AND_ATTRIBUTES) Privileges[1]" ;ANYSIZE_ARRAY:=1
        ,tkp:=Struct(TOKEN_PRIVILEGES)	
	; we are running NT/2k/XP, make sure we have rights to shutdown
  ; Get a token for this process.
  if (!DllCall("OpenProcessToken","PTR",DllCall("GetCurrentProcess","PTR"),"UInt", TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,"PTR*", hToken)) 
    return ErrorLevel:=1,""						; Don't have the rights
  ; Get the LUID for the shutdown privilege.
  DllCall("LookupPrivilegeValue","PTR",0,"Str","SeSecurityPrivilege","PTR", tkp.Privileges.1.Luid[""]) 
  tkp.PrivilegeCount := 1  ; one privilege to set
  tkp.Privileges.1.Attributes := SE_PRIVILEGE_ENABLED 
  ; Get the shutdown privilege for this process.
  DllCall("AdjustTokenPrivileges","PTR",hToken,"Char", 0,"PTR", tkp[],"UInt", 0,"PTR", 0,"UInt", 0) 
	; ExitWindows
  If !DllCall("GetLastError")
    ErrorLevel:=!DllCall("ExitWindowsEx","UInt",nFlag,"UInt", 0)
  else ErrorLevel:=1	; Don't have the rights
}
