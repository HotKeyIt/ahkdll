ResExist(dll,name,type:=10){
static TH32CS_SNAPMODULE:=0x00000008,MODULEENTRY32:="DWORD dwSize;DWORD th32ModuleID;DWORD th32ProcessID;DWORD GlblcntUsage;DWORD ProccntUsage;BYTE *modBaseAddr;DWORD modBaseSize;HMODULE hModule;TCHAR szModule[256];TCHAR szExePath[260]",me32 := Struct(MODULEENTRY32,{dwSize:sizeof(MODULEENTRY32)}),find:=DynaCall("FindResourceW","t=tss"),lib:=DynaCall("LoadLibraryW","t=w"),free:=DynaCall("FreeLibrary","t"),snap:=DynaCall("CreateToolhelp32Snapshot","t=uit",TH32CS_SNAPMODULE,DllCall("GetCurrentProcessId")),first:=DynaCall("Module32FirstW","tt"),next:=DynaCall("Module32NextW","tt"),close:=DynaCall("CloseHandle","t"),fullpath,init:=VarSetCapacity(fullpath,520),getpath:=DynaCall("GetFullPathNameW","suitt","",260,&fullpath)
If !InStr(dll,"\")
getpath[dll]
else fullpath:=dll
If (hSnap:=snap[]) && first[hSnap,me32[]]
while (A_Index=1 || next[hSnap,me32[]])
if StrGet(me32.szExePath[""])=dll && hModule:= me32.modBaseAddr["",""],break
if hSnap,close[hSnap]
if !hModule,return Exist:=!!find[hModule:=lib[dll],name,type],free[hModule],Exist
else return !!find[hModule:=lib[dll],name,type]
}