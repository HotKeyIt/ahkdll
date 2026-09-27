ResGet(ByRef data,dll,name,type:=10,lang:=""){
  static TH32CS_SNAPMODULE,MODULEENTRY32,me32,fullpath
  if !TH32CS_SNAPMODULE
    TH32CS_SNAPMODULE:=0x00000008,MODULEENTRY32:="DWORD dwSize;th32ModuleID;th32ProcessID;GlblcntUsage;ProccntUsage;BYTE *modBaseAddr;DWORD modBaseSize;HMODULE hModule;WCHAR szModule[256];WCHAR szExePath[260]"
    ,me32 := Struct(MODULEENTRY32),me32.dwSize:=sizeof(MODULEENTRY32),VarSetCapacity(fullpath,520)
	GetFullPathName(dll,260,fullpath),VarSetCapacity(fullpath,-1)
	hModule:=0,loaded:=0,sz:=""
	If (hSnap:=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId())) && Module32FirstW(hSnap,me32[])
		while (A_Index=1 || Module32NextW(hSnap,me32[]))
			if StrGet(me32.szExePath[""],"UTF-16")=fullpath && hModule:= me32.hModule
        break
  if hSnap
		CloseHandle(hSnap)
  if !hModule && !loaded:=hModule:=LoadLibrary(fullpath)
		return 0
  if (hResource:=lang=""?DllCall("FindResource","PTR",hModule,name+0=""?"Str":"PTR",name,type+0=""?"Str":"PTR",type,"PTR"):DllCall("FindResourceEx","PTR",hModule,type+0=""?"Str":"PTR",type,name+0=""?"Str":"PTR",name,"UShort",lang,"PTR"))
      && pdata:=LockResource(hResData:=LoadResource(hModule,hResource))
      VarsetCapacity(data,2+sz:=SizeofResource(hModule,hResource),0),RtlMoveMemory(&data,pData,sz)
	if loaded
		FreeLibrary(hModule)
	return sz
}