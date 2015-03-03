A_IsAdmin(b:=0,n:=0){
	static Open:=DynaCall("Advapi32\OpenSCManagerW","ssui",0,0,8),Lock:=DynaCall("Advapi32\LockServiceDatabase","t")
				,UnLock:=DynaCall("Advapi32\UnLockServiceDatabase","t"),Close:=DynaCall("Advapi32\CloseServiceHandle","t"),ERROR_SERVICE_DATABASE_LOCKED:=1055
Critical
	if !b&&n
		return 1
	If h:=Open[]
		If Lock[h]
			Unlock[h],Close[h],h:=1
		else if A_LastError!=ERROR_SERVICE_DATABASE_LOCKED
			Close[h],h:=0
		else Close[h],h:=1
	return b?StrPut(h,b):h
}