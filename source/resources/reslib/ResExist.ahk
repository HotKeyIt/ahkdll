ResExist(dll,name,type:=10,lang:=""){
static TH32CS_SNAPMODULE:=0x00000008,MODULEENTRY32:="DWORD dwSize;DWORD th32ModuleID;DWORD th32ProcessID;DWORD GlblcntUsage;DWORD ProccntUsage;BYTE *modBaseAddr;DWORD modBaseSize;HMODULE hModule;TCHAR szModule[256];TCHAR szExePath[260]",me32 := Struct(MODULEENTRY32,,{dwSize:sizeof(MODULEENTRY32)}),fullpath:=BufferAlloc(520)
GetFullPathNameW(dll,260,fullpath)
If (hSnap:=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId())) && Module32FirstW(hSnap,me32[])
  while (A_Index=1 || Module32NextW(hSnap,me32[]))
    if StrGet(me32.szExePath[""])=dll && hModule:= me32.hModule
      break
if hSnap
  CloseHandle(hSnap)
if !hModule && !hModule:=LoadLibrary(StrGet(fullpath))
  return 0
hResource:=lang=""?FindResourceW(hModule,name,type):FindResourceExW(hModule,name,type,lang)
if (hModule != me32.hModule)
  FreeLibrary(hModule)
return !!hResource
}