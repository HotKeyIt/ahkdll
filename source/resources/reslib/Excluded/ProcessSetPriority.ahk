ProcessSetPriority(aProcess:="",aPriority:="N"){
	static IDLE_PRIORITY_CLASS := 0x00000040,BELOW_NORMAL_PRIORITY_CLASS := 0x00004000,NORMAL_PRIORITY_CLASS := 0x00000020
				,ABOVE_NORMAL_PRIORITY_CLASS := 0x00008000,HIGH_PRIORITY_CLASS := 0x00000080,REALTIME_PRIORITY_CLASS := 0x00000100
				,L:=IDLE_PRIORITY_CLASS,B:=BELOW_NORMAL_PRIORITY_CLASS,N:=NORMAL_PRIORITY_CLASS,A:=ABOVE_NORMAL_PRIORITY_CLASS
				,H:=HIGH_PRIORITY_CLASS,R:=REALTIME_PRIORITY_CLASS
				,PROCESS_SET_INFORMATION:=512,TH32CS_SNAPPROCESS:=2,MAX_PATH:=260
        ,PROCESSENTRY32 :="DWORD dwSize;DWORD cntUsage;DWORD th32ProcessID;ULONG_PTR th32DefaultHeapID;DWORD th32ModuleID;DWORD cntThreads;DWORD th32ParentProcessID;LONG pcPriClassBase;DWORD dwFlags;WCHAR szExeFile[" MAX_PATH "]"
        ,proc:=Struct(PROCESSENTRY32)
	If !InStr(" L B N A H R "," " aPriority " ")
		return
	else priority:=%aPriority%
	if aProcess {
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
  } else
    pid := DllCall("GetCurrentProcessId")
	if pid {  ; Assign
		if (hProcess := DllCall("OpenProcess","UInt",PROCESS_SET_INFORMATION,"UInt", FALSE,"UInt", pid,"PTR")) ; Assign
		{
			; If OS doesn't support "above/below normal", seems best to default to normal rather than high/low,
			; since "above/below normal" aren't that dramatically different from normal:
			if DllCall("SetPriorityClass","PTR",hProcess,"UInt", priority)
				return,DllCall("CloseHandle","PTR",hProcess), pid ; Indicate success.
			else return
		}
	}
	; Otherwise, return a PID of 0 to indicate failure.
	return
}
