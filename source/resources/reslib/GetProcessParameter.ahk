GetProcessParameter(dwProcId, parameter:="cmd_line") {
static PROCESS_BASIC_INFORMATION :="PVOID Reserved1;PEB *PebBaseAddress;PVOID Reserved2[2];ULONG_PTR UniqueProcessId;PVOID Reserved3;",PEB :="BYTE Reserved1[2];BYTE BeingDebugged;BYTE Reserved2[1];PVOID Reserved3[2];PEB_LDR_DATA *Ldr;RTL_USER_PROCESS_PARAMETERS *ProcessParameters;PVOID Reserved4[3];PVOID AtlThunkSListPtr;PVOID Reserved5;ULONG Reserved6;PVOID Reserved7;ULONG Reserved8;ULONG AtlThunkSListPtr32;PVOID Reserved9[45];BYTE Reserved10[96];PTR* PostProcessInitRoutine;BYTE Reserved11[128];PVOID Reserved12[1];ULONG SessionId;",PEB_LDR_DATA:="BYTE Reserved1[8];PVOID Reserved2[3];LIST_ENTRY InMemoryOrderModuleList;",LIST_ENTRY :="LIST_ENTRY *Flink;LIST_ENTRY *Blink;",RTL_USER_PROCESS_PARAMETERS :="BYTE Reserved1[16];PVOID Reserved2[10];UNICODE_STRING ImagePathName;UNICODE_STRING CommandLine;",UNICODE_STRING :="USHORT Length;USHORT MaximumLength;PTR  Buffer;",MY_RTL_DRIVE_LETTER_CURDIR:="USHORT Flags;USHORT Length;ULONG TimeStamp;LPTSTR DosPath;",MY_CURDIR :="UNICODE_STRING DosPath;VOID* Handle;",MY_RTL_USER_PROCESS_PARAMETERS :="ULONG MaximumLength;ULONG Length;ULONG Flags;ULONG DebugFlags;VOID* ConsoleHandle;ULONG ConsoleFlags;VOID* StandardInput;VOID* StandardOutput;VOID* StandardError;MY_CURDIR CurrentDirectory;UNICODE_STRING DllPath;UNICODE_STRING ImagePathName;UNICODE_STRING CommandLine;VOID* Environment;ULONG StartingX;ULONG StartingY;ULONG CountX;ULONG CountY;ULONG CountCharsX;ULONG CountCharsY;ULONG FillAttribute;ULONG WindowFlags;ULONG ShowWindowFlags;UNICODE_STRING WindowTitle;UNICODE_STRING DesktopInfo;UNICODE_STRING ShellInfo;UNICODE_STRING RuntimeData;MY_RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];"
	static pbi := Struct(PROCESS_BASIC_INFORMATION)

	if (hProcess := OpenProcess(1024 | 16, FALSE, dwProcId)){ ;PROCESS_QUERY_INFORMATION:=1024 | PROCESS_VM_READ:=16
		if (pbi := !DllCall("ntdll.dll\NtQueryInformationProcess","Ptr",hProcess, "Ptr", 0, "PTR", pbi[], "Uint", sizeof(pbi),"PTR", NULL) ? pbi : 0) {
			if parameter="parent_pid"
				return (CloseHandle(hProcess), pbi.Reserved3)
			pRtlUserProcParamsAddress:=Struct("MY_RTL_USER_PROCESS_PARAMETERS*")
			if ReadProcessMemory(hProcess, pbi["PebBaseAddress"]["ProcessParameters"][""], pRtlUserProcParamsAddress[], sizeof(pRtlUserProcParamsAddress), getvar(nSizeRead:=0)) && (nSizeRead == sizeof(pRtlUserProcParamsAddress)){
				paramStr := Struct(UNICODE_STRING)
				if (ReadProcessMemory(hProcess, parameter="working_dir"?pRtlUserProcParamsAddress[1]["CurrentDirectory"]["DosPath"][""]:pRtlUserProcParamsAddress[1]["CommandLine"][""],paramStr[], sizeof(paramStr), getvar(nSizeRead)) && (nSizeRead == sizeof(paramStr))) {
					szParameter := Buffer(paramStr["MaximumLength"])
					if (szParameter) {
						if (ReadProcessMemory(hProcess, paramStr["buffer"], szParameter.ptr, paramStr["Length"], getvar(nSizeRead)) && (nSizeRead == paramStr["Length"]))
							return (CloseHandle(hProcess),StrGet(szParameter,paramStr["Length"]//2))
						else
							throw ValueError("Couldn't read parameter string",-1)
					} else
						throw ValueError("Couldn't allocate memory",-1)
				} else
					throw ValueError("Couldn't read parameter",-1)
			} else
				throw ValueError("Couldn't read process memory",-1)
		} else
			throw ValueError("Couldn't get PEB address",-1)
		
	} else
		throw ValueError("Couldn't open process",-1)
}