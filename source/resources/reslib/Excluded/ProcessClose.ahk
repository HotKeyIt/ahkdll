ProcessClose(aProcess){
  static PROCESS_ALL_ACCESS:=2035711,TH32CS_SNAPPROCESS:=2,MAX_PATH:=260
        ,PROCESSENTRY32 :="DWORD dwSize;DWORD cntUsage;DWORD th32ProcessID;ULONG_PTR th32DefaultHeapID;DWORD th32ModuleID;DWORD cntThreads;DWORD th32ParentProcessID;LONG pcPriClassBase;DWORD dwFlags;WCHAR szExeFile[" MAX_PATH "]"
        ,proc:=Struct(PROCESSENTRY32)
  proc.Fill(),proc.dwSize := sizeof(proc)
	,snapshot := DllCall("CreateToolhelp32Snapshot","UInt",TH32CS_SNAPPROCESS,"UInt",0,"PTR")
	,DllCall("Process32First","PTR",snapshot,"PTR", proc[])

	; Determine the PID if aProcess is a pure, non-negative integer (any negative number
	; is more likely to be the name of a process [with a leading dash], rather than the PID).
	,specified_pid := Type(aProcess)="Integer" ? aProcess : 0

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
  if pid  ; Assign
    if (hProcess := DllCall("OpenProcess","UInt",PROCESS_ALL_ACCESS,"UInt", FALSE,"UInt", pid,"PTR")){
      if !DllCall("TerminateProcess","PTR",hProcess,"UInt", 0)
        pid:=0
      return DllCall("CloseHandle","PTR",hProcess), pid ; Indicate success.
    }
  ; Since above didn't return, yield a PID of 0 to indicate failure.
  return 0
}