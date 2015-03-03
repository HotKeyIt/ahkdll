ProcessWaitClose(aProcess,timeout:=0){
  static SLEEP_INTERVAL:=10,SLEEP_INTERVAL_HALF:=SLEEP_INTERVAL/2
        ,TH32CS_SNAPPROCESS:=2,MAX_PATH:=260
        ,PROCESSENTRY32 :="DWORD dwSize;DWORD cntUsage;DWORD th32ProcessID;ULONG_PTR th32DefaultHeapID;DWORD th32ModuleID;DWORD cntThreads;DWORD th32ParentProcessID;LONG pcPriClassBase;DWORD dwFlags;WCHAR szExeFile[" MAX_PATH "]"
        ,proc:=Struct(PROCESSENTRY32)
  ; This section is similar to that used for WINWAIT and RUNWAIT:
  if (timeout) ; The param containing the timeout value was specified.
  {
    wait_indefinitely := false
    ,sleep_duration := timeout * 1000 ; Can be zero.
    ,start_time := A_TickCount
  }
  else
  {
    wait_indefinitely := true
    ,sleep_duration := 0 ; Just to catch any bugs.
  }
  ; Determine the PID if aProcess is a pure, non-negative integer (any negative number
  ; is more likely to be the name of a process [with a leading dash], rather than the PID).
  specified_pid := Type(aProcess)="Integer" ? aProcess : 0
  Loop { ; Always do the first iteration so that at least one check is done.
    pid := 0,proc.Fill(),proc.dwSize := sizeof(proc)
  	,snapshot := DllCall("CreateToolhelp32Snapshot","UInt",TH32CS_SNAPPROCESS,"UInt",0,"PTR")
  	,DllCall("Process32First","PTR",snapshot,"PTR", proc[])
  	while DllCall("Process32Next","PTR",snapshot,"PTR",proc[]){
  		if (specified_pid && specified_pid = proc.th32ProcessID && pid:=specified_pid)
        break
  		; Otherwise, check for matching name even if aProcess is purely numeric (i.e. a number might
  		; also be a valid name?):
  		; It seems that proc.szExeFile never contains a path, just the executable name.
  		; But in case it ever does, ensure consistency by removing the path:
  		SplitPath,% StrGet(proc.szExeFile[""],"CP0"),szFile,,,szFileNoExt
  		if (szFile = aProcess || szFileNoExt = aProcess){ ; lstrcmpi() is not used: 1) avoids breaking existing scripts 2) provides consistent behavior across multiple locales 3) performance.
  			pid:=proc.th32ProcessID
        break
      }
  	}
  	DllCall("CloseHandle","PTR",snapshot)
    if (pid = 0) ; i.e. condition of this cmd is satisfied.
    {
      ; For WaitClose: Since PID cannot always be determined (i.e. if process never existed,
      ; there was no need to wait for it to close), for consistency, return 0 on success.
      return pid
    }
    ; Must cast to int or any negative result will be lost due to DWORD type:
    if (wait_indefinitely || (sleep_duration - A_TickCount - start_time > SLEEP_INTERVAL_HALF))
    {
      Sleep 100  ; For performance reasons, don't check as often as the WinWait family does.
    }
    else ; Done waiting.
    {
      ; Return 0 if "Process Wait" times out or the PID of the process that still exists
      ; if "Process WaitClose" times out.
      return pid
    }
  } ; Loop
}