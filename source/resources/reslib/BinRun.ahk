BinRun(pData,cmdLine:="",cmdLineScript:="",Hide:=0,ExeToUse:=""){
  static IMAGE_DOS_HEADER :="WORD e_magic;WORD e_cblp;WORD e_cp;WORD e_crlc;WORD e_cparhdr;WORD e_minalloc;WORD e_maxalloc;WORD e_ss;WORD e_sp;WORD e_csum;WORD e_ip;WORD e_cs;WORD e_lfarlc;WORD e_ovno;WORD e_res[4];WORD e_oemid;WORD e_oeminfo;WORD e_res2[10];LONG e_lfanew"
    ,IMAGE_FILE_HEADER :="WORD Machine;WORD NumberOfSections;DWORD TimeDateStamp;DWORD PointerToSymbolTable;DWORD NumberOfSymbols;WORD SizeOfOptionalHeader;WORD Characteristics"
    ,IMAGE_DATA_DIRECTORY :="DWORD VirtualAddress;DWORD Size"
    ,IMAGE_OPTIONAL_HEADER64:="WORD Magic;BYTE MajorLinkerVersion;BYTE MinorLinkerVersion;DWORD SizeOfCode;DWORD SizeOfInitializedData;DWORD SizeOfUninitializedData;DWORD AddressOfEntryPoint;DWORD BaseOfCode;ULONGLONG ImageBase;DWORD SectionAlignment;DWORD FileAlignment;WORD MajorOperatingSystemVersion;WORD MinorOperatingSystemVersion;WORD MajorImageVersion;WORD MinorImageVersion;WORD MajorSubsystemVersion;WORD MinorSubsystemVersion;DWORD Win32VersionValue;DWORD SizeOfImage;DWORD SizeOfHeaders;DWORD CheckSum;WORD Subsystem;WORD DllCharacteristics;ULONGLONG SizeOfStackReserve;ULONGLONG SizeOfStackCommit;ULONGLONG SizeOfHeapReserve;ULONGLONG SizeOfHeapCommit;DWORD LoaderFlags;DWORD NumberOfRvaAndSizes;BinRun(IMAGE_DATA_DIRECTORY) DataDirectory[16]"
    ,IMAGE_OPTIONAL_HEADER32 :="WORD Magic;BYTE MajorLinkerVersion;BYTE MinorLinkerVersion;DWORD SizeOfCode;DWORD SizeOfInitializedData;DWORD SizeOfUninitializedData;DWORD AddressOfEntryPoint;DWORD BaseOfCode;DWORD BaseOfData;DWORD ImageBase;DWORD SectionAlignment;DWORD FileAlignment;WORD MajorOperatingSystemVersion;WORD MinorOperatingSystemVersion;WORD MajorImageVersion;WORD MinorImageVersion;WORD MajorSubsystemVersion;WORD MinorSubsystemVersion;DWORD Win32VersionValue;DWORD SizeOfImage;DWORD SizeOfHeaders;DWORD CheckSum;WORD Subsystem;WORD DllCharacteristics;DWORD SizeOfStackReserve;DWORD SizeOfStackCommit;DWORD SizeOfHeapReserve;DWORD SizeOfHeapCommit;DWORD LoaderFlags;DWORD NumberOfRvaAndSizes;BinRun(IMAGE_DATA_DIRECTORY) DataDirectory[16]"
    ,IMAGE_NT_HEADERS:="DWORD Signature;BinRun(IMAGE_FILE_HEADER) FileHeader;BinRun(IMAGE_OPTIONAL_HEADER" (A_PtrSize=8?64:32) ") OptionalHeader"
    ,IMAGE_NT_HEADERS32:="DWORD Signature;BinRun(IMAGE_FILE_HEADER) FileHeader;BinRun(IMAGE_OPTIONAL_HEADER32) OptionalHeader"
    ,IMAGE_NT_HEADERS64:="DWORD Signature;BinRun(IMAGE_FILE_HEADER) FileHeader;BinRun(IMAGE_OPTIONAL_HEADER64) OptionalHeader"
    ,IMAGE_SECTION_HEADER:="BYTE Name[8];{DWORD PhysicalAddress;DWORD VirtualSize};DWORD VirtualAddress;DWORD SizeOfRawData;DWORD PointerToRawData;DWORD PointerToRelocations;DWORD PointerToLinenumbers;WORD NumberOfRelocations;WORD NumberOfLinenumbers;DWORD Characteristics"
    ,FLOATING_SAVE_AREA :="DWORD ControlWord;DWORD StatusWord;DWORD TagWord;DWORD ErrorOffset;DWORD ErrorSelector;DWORD DataOffset;DWORD DataSelector;BYTE RegisterArea[80];DWORD Cr0NpxState"
    ,PROCESS_INFORMATION :="HANDLE hProcess;HANDLE hThread;DWORD dwProcessId;DWORD dwThreadId"
    ,STARTUPINFO :="DWORD cb;LPTSTR lpReserved;LPTSTR lpDesktop;LPTSTR lpTitle;DWORD dwX;DWORD dwY;DWORD dwXSize;DWORD dwYSize;DWORD dwXCountChars;DWORD dwYCountChars;DWORD dwFillAttribute;DWORD dwFlags;WORD wShowWindow;WORD cbReserved2;LPBYTE lpReserved2;HANDLE hStdInput;HANDLE hStdOutput;HANDLE hStdError"
    ,M128A:="ULONGLONG Low;LONGLONG High"
    ,_XMM_SAVE_AREA32 :="WORD ControlWord;WORD StatusWord;BYTE TagWord;BYTE Reserved1;WORD ErrorOpcode;DWORD ErrorOffset;WORD ErrorSelector;WORD Reserved2;DWORD DataOffset;WORD DataSelector;WORD Reserved3;DWORD MxCsr;DWORD MxCsr_Mask;BinRun(M128A) FloatRegisters[8];BinRun(M128A) XmmRegisters[16];BYTE Reserved4[96]"
    ,CONTEXT64:="DWORD64 P1Home;DWORD64 P2Home;DWORD64 P3Home;DWORD64 P4Home;DWORD64 P5Home;DWORD64 P6Home;DWORD ContextFlags;DWORD MxCsr;WORD SegCs;WORD SegDs;WORD SegEs;WORD SegFs;WORD SegGs;WORD SegSs;DWORD EFlags;DWORD64 Dr0;DWORD64 Dr1;DWORD64 Dr2;DWORD64 Dr3;DWORD64 Dr6;DWORD64 Dr7;DWORD64 Rax;DWORD64 Rcx;DWORD64 Rdx;DWORD64 Rbx;DWORD64Rsp;DWORD64 Rbp;DWORD64 Rsi;DWORD64 Rdi;DWORD64 R8;DWORD64 R9;DWORD64 R10;DWORD64 R11;DWORD64R12;DWORD64 R13;DWORD64 R14;DWORD64 R15;DWORD64 Rip;{BinRun(_XMM_SAVE_AREA32) FltSave;struct { BinRun(M128A) Header[2];BinRun(M128A) Legacy[8];BinRun(M128A) Xmm0;BinRun(M128A) Xmm1;BinRun(M128A) Xmm2;BinRun(M128A) Xmm3;BinRun(M128A) Xmm4;BinRun(M128A) Xmm5;BinRun(M128A) Xmm6;BinRun(M128A) Xmm7;BinRun(M128A) Xmm8;BinRun(M128A) Xmm9;BinRun(M128A) Xmm10;BinRun(M128A) Xmm11;BinRun(M128A) Xmm12;BinRun(M128A) Xmm13;BinRun(M128A) Xmm14;BinRun(M128A) Xmm15}};BinRun(M128A) VectorRegister[26];DWORD64 VectorControl;DWORD64 DebugControl;DWORD64 LastBranchToRip;DWORD64 LastBranchFromRip;DWORD64 LastExceptionToRip;DWORD64 LastExceptionFromRip"
    ,CONTEXT32:="DWORD ContextFlags;DWORD Dr0;DWORD Dr1;DWORD Dr2;DWORD Dr3;DWORD Dr6;DWORD Dr7;BinRun(FLOATING_SAVE_AREA) FloatSave;DWORD SegGs;DWORD SegFs;DWORD SegEs;DWORD SegDs;DWORD Edi;DWORD Esi;DWORD Ebx;DWORD Edx;DWORD Ecx;DWORD Eax;DWORD Ebp;DWORD Eip;DWORD SegCs;DWORD EFlags;DWORD Esp;DWORD SegSs;BYTE ExtendedRegisters[512]"
    ,IMAGE_NT_SIGNATURE:=17744,IMAGE_DOS_SIGNATURE:=23117,PAGE_EXECUTE_READWRITE:=64,CREATE_SUSPENDED:=4
    ,MEM_COMMIT:=4096,MEM_RESERVE:=8192,STARTF_USESHOWWINDOW:=1
    ,h2o:="B29C2D1CA2C24A57BC5E208EA09E162F(){`nPLACEHOLDERB29C2D1CA2C24A57BC5E208EA09E162Fh:='',dmp:=BufferAlloc(sz:=StrLen(hex)//2,0)`nLoop Parse,hex`nIf (`"`"!=h.=A_LoopField) && !Mod(A_Index,2)`nNumPut(`"UChar`",`"0x`" h,dmp,A_Index/2-1),h:=`"`"`nreturn ObjLoad(dmp.Ptr)`n}`n"
  local data:="",data2:="",dmp:="", force32bit:=0, script:="",_s:=""
  if (Type(pData)="Buffer")
    data:=pData,pData:=data.Ptr
  else If (Type(pData)!="Integer")
  {	
    ; Try first reading the file from Resource
    If res := FindResource(lib:=GetModuleHandle(),pData,10)
      data:=BufferAlloc(sz :=SizeofResource(lib,res))
      ,RtlMoveMemory(data.Ptr,LockResource(hres:=LoadResource(lib,res)),sz)
      ,FreeResource(hres)
      ,data2:=UnZipRawMemory(data.Ptr,sz)?(data:=data2):""
    else ; else try reading file from disc etc...
      Data:=FileRead(pData,"RAW")
    pData:=Data.Ptr
  }
  
  If InStr(cmdLine,"`n"){ ; a script was given, first line contains the cmdLine
    __PIPE_GA_ := CreateNamedPipe(PipeName := "\\.\pipe\AHK" A_TickCount,2,0,255),__PIPE_ := CreateNamedPipe(PipeName,2,0,255)
    if (__PIPE_=-1 || __PIPE_GA_=-1)   
      Return 0
    Script:=SubStr(cmdLine,InStr(cmdLine,"`n")+1),cmdLine:=Trim(SubStr(cmdLine,1,InStr(cmdLine,"`n")),"`n`r") A_Space PipeName
  }

  IDH:=Struct(IMAGE_DOS_HEADER,pData)
  if (IDH.e_magic != IMAGE_DOS_SIGNATURE){
    MsgBox("ERROR: e_magic not found")
    return
  }
  INH := Struct(IMAGE_NT_HEADERS,pData + IDH.e_lfanew)
  
  if (INH.Signature != IMAGE_NT_SIGNATURE){
    MsgBox("ERROR: Signature not found")
    return
  }
  
  If (A_PtrSize=8&&INH.OptionalHeader.magic=267) ; x86 on x64
    pNtHeader:=Struct(IMAGE_NT_HEADERS32,pData + IDH.e_lfanew),ctx:=Struct(Context32),Force32Bit:=1
    ,ctx.ContextFlags := (A_PtrSize=8?0x100000:0x10000) | 0x2 ;CONTEXT_INTEGER
    ,UsedExe:=ExeToUse?ExeToUse:A_WinDir "\Microsoft.NET\Framework\v2.0.50727\vbc.exe"
  else if (A_PtrSize=4&&INH.OptionalHeader.magic=523) ; x64 on x86 not possible
    Return false
  else 
    pNtHeader:=INH,UsedExe:=ExeToUse?ExeToUse:A_IsCompiled?A_ScriptFullPath:A_AhkPath
    ,ctx:=Struct(A_PtrSize=8?Context64:Context32),ctx.ContextFlags := (A_PtrSize=8?0x100000:0x10000) | 0x2 ;CONTEXT_INTEGER
  pi:=Struct(PROCESS_INFORMATION)
  ,si:=Struct(STARTUPINFO),si.cb:=sizeof(si),si.dwFlags:=HIDE?STARTF_USESHOWWINDOW:0 ;si.wShowWindow already set to 0
  if CreateProcess(0,"`"" UsedExe "`"" A_Space cmdLine (cmdLineScript?A_Space (IsObject(cmdLineScript)?"":cmdLineScript):""),0,0,0,CREATE_SUSPENDED,0,0,si[],pi[]){
      if (Force32Bit && Wow64GetThreadContext(pi.hThread,ctx[])) || (!Force32Bit && GetThreadContext(pi.hThread,ctx[])){
          pPebImageBase:=ctx[A_PtrSize=8&&!Force32Bit?"Rdx":"Ebx"] + (Force32Bit?4:A_PtrSize)*2
          if ReadProcessMemory(pi.hProcess, pPebImageBase, getvar(dwImagebase:=0), (Force32Bit?4:A_PtrSize), getvar(NumberOfBytes:=0)){
              DllCall("ntdll\NtUnmapViewOfSection","PTR",pi.hProcess, "PTR",dwImagebase)
              pImagebase := VirtualAllocEx(pi.hProcess, pNtHeader.OptionalHeader.ImageBase, pNtHeader.OptionalHeader.SizeOfImage, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE)
              if (pImagebase)
              {
                  if WriteProcessMemory(pi.hProcess, pImagebase, pData, pNtHeader.OptionalHeader.SizeOfHeaders, getvar(NumberOfBytes:=0)){
                      pSecHeader :=Struct(IMAGE_SECTION_HEADER)
                      ,off:=pNtHeader.Offset("OptionalHeader")
                      ,vvv:=pNtHeader.OptionalHeader[""]
                      ,aaa:=pNTHeader[]
                      ,pSecHeader[] :=pNtHeader.OptionalHeader[""]+pNtHeader.FileHeader.SizeOfOptionalHeader
                      ,counter := 0
                      while (++counter < pNtHeader.FileHeader.NumberOfSections+1){
                          WriteProcessMemory(pi.hProcess, pImagebase + pSecHeader.VirtualAddress, pData + pSecHeader.PointerToRawData, pSecHeader.SizeOfRawData, getvar(NumberOfBytes:=0))
                          pSecHeader[]:=pSecHeader[]+sizeof(pSecHeader)
                      }
                      if WriteProcessMemory(pi.hProcess, pPebImageBase, pNtHeader.OptionalHeader.ImageBase[""], (Force32Bit?4:A_PtrSize), getvar(NumberOfBytes:=0)){
                          ctx[A_PtrSize=8&&!Force32Bit?"Rcx":"Eax"] := pImagebase + pNtHeader.OptionalHeader.AddressOfEntryPoint
                          if (Force32Bit && Wow64SetThreadContext(pi.hThread, ctx[])) || (!Force32Bit && SetThreadContext(pi.hThread, ctx[])){
                              if ResumeThread(pi.hThread){
                                if (Script){ ; use pipe to pass script to new executable
                                  If IsObject(cmdLineScript){
                                    sz:=ObjDump(cmdLineScript,dmp),hex:=BinToHex(dmp.Ptr,sz)
                                    While _hex:=SubStr(Hex,1 + (A_Index-1)*16370,16370)
                                      _s.= "hex" (A_Index=1?":":".") "=`"" _hex "`"`n"
                                    script:=chr(0xfeff) StrReplace(h2o,"PLACEHOLDERB29C2D1CA2C24A57BC5E208EA09E162F",_s) "A_Args:=B29C2D1CA2C24A57BC5E208EA09E162F()`n" script
                                  } else script:=chr(0xfeff) script
                                  ConnectNamedPipe(__PIPE_GA_),CloseHandle(__PIPE_GA_),ConnectNamedPipe(__PIPE_)
                                  if !WriteFile(__PIPE_, StrPtr(script), StrLen(script)*2,getvar(NumberOfBytes:=0))
                                    Return (CloseHandle(__PIPE_),0)
                                  CloseHandle(__PIPE_)
                                }
                                return pi.dwProcessId
                              }
                          }
                      }
                  }
              }
          }
      }
      TerminateProcess(pi.hProcess)
  }
  return FALSE
}