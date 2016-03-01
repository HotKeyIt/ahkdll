If (InStr(A_AhkVersion,"1")=1) {
  MsgBox Requires AutoHotkey V2 version, script will exit now!
  ExitApp
}

SetWorkingDir,% A_ScriptDir

dirs:=[A_ScriptDir "\bin"]
subs:={"Win32w":1,"x64w":1}
exts:=["lib","exp","pdb","iobj","ipdb"]
for t1,dir in dirs
	for t2,ext in exts
		LoopFiles % dir "\*." ext,FR
			If subs.HasKey(SubStr(A_LoopFileDir,InStr(A_LoopFileDir,"\",1,-1)+1))
				FileDelete % A_LoopFileFullPath
for t1,dir in dirs
	LoopFiles % dir "\AutoHotkeyDll.dll",FR
		If subs.HasKey(SubStr(A_LoopFileDir,InStr(A_LoopFileDir,"\",1,-1)+1))
			FileMove,% A_LoopFileFullPath,% RegExReplace(A_LoopFileFullPath,"i)AutoHotkeyDll\.dll","AutoHotkey.dll"),1

RCData:={("bin\Win32w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"],("bin\x64w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"]}

IniRead,L_VersionOld,% A_ScriptDir "\AlphaVersion.ini",Version,L_Version
IniRead,H_VersionOld,% A_ScriptDir "\AlphaVersion.ini",Version,H_Version
MsgBox, 260, Version, Increase Version?,10
If (A_MsgBoxResult="Yes"||A_MsgBoxResult="TimeOut"){
	LoopFiles, D:\AutoHotkey\AutoHotkey_L 2\*,D
	  if SubStr(A_LoopFileFullPath,-3)>L_version
		L_version:=SubStr(A_LoopFileFullPath,-3)
	If L_VersionOld<L_Version
	  H_Version:="001"
	else H_Version:=H_VersionOld+1
	While StrLen(L_Version)<3
	  L_Version:="0" L_Version
	While StrLen(H_Version)<3
	  H_Version:="0" H_Version
	IniWrite,% L_Version,% A_ScriptDir "\AlphaVersion.ini",Version,L_Version
	IniWrite,% H_Version,% A_ScriptDir "\AlphaVersion.ini",Version,H_Version
} else L_Version:=L_VersionOld, H_Version:=H_VersionOld
for k,v in RCData
  LoopFiles % A_ScriptDir "\" k "\*.dll"
  {
    if !hUpdate:=BeginUpdateResource(A_LoopFileFullPath)
    {
      MsgBox % "Error Begin: " A_LoopFileFullPath "`n" ErrMsg()
      ExitApp
    }
    ChangeVersionInfo(A_LoopFileFullPath, hUpdate, {version:"2.0-a" L_version "-H" H_version})
    If !EndUpdateResource(hUpdate,0)
    {
      MsgBox % "End: " ErrMsg()
      ExitApp
    }
  }
Loop 2 {
  idx:=A_Index
  for k,o in RCData
  {
    sourcedir:=A_ScriptDir "\" k "\"
    exe:=sourcedir "AutoHotkey" (idx=1?".exe":"SC.bin")
    if !hUpdate:=BeginUpdateResource(exe)
    {
      MsgBox % "Error Begin: " exe "`n" ErrMsg()
      ExitApp
    }
    ChangeVersionInfo(exe, hUpdate, {version:"2.0-a" L_version "-H" H_version})
    for k,v in o
    {
	  FileRead, data,% "*c " sourcedir "\" v
	  FileGetSize, sz,% sourcedir "\" v
	  sz:=ZipRawMemory(&data, sz, var)
      vres:=v="AutoHotkey.dll"?"F903E44B8A904483A1732BA84EA6191F":v="AutoHotkeyMini.dll"?"FC2328B39C194A4788051A3B01B1E7D5":StrUpper(v)
      if FindResource(hUpdate,10,vres)
        If !UpdateResource(hUpdate,10,vres,1033)
          MsgBox % "Delete: " v "-" ErrMsg()
      If !UpdateResource(hUpdate,10,vres,1033,&var,sz)
        MsgBox % "Update: " v "-" ErrMsg()
    }
    if idx=1
    LoopFiles,% A_ScriptDir "\source\resources\reslib\*.ahk"
    {
	  FileRead, data,% "*c " A_ScriptDir "\source\resources\reslib\" A_LoopFileName
	  FileGetSize, sz,% A_ScriptDir "\source\resources\reslib\" A_LoopFileName
	  sz:=ZipRawMemory(&data, sz, var)
      if FindResource(hUpdate,"LIB",StrUpper(A_LoopFileName))
        If !UpdateResource(hUpdate,"LIB",StrUpper(A_LoopFileName),1033)
          MsgBox % "Delete: " StrUpper(A_LoopFileName) "-" ErrMsg()
      If !UpdateResource(hUpdate,"LIB",StrUpper(A_LoopFileName),1033,&var,sz)
        MsgBox % "Update: " StrUpper(A_LoopFileName) "-" ErrMsg()
    }
    If !EndUpdateResource(hUpdate,0)
    {
      MsgBox % "End: " ErrMsg()
      ExitApp
    }
  }
}
MsgBox Finished


class VersionRes
{
	Name := ""
	,Data := ""
	,IsText := true
	,DataSize := 0
	,Children := []
	
	__New(addr := 0)
	{
		if !addr
			return this
		
		wLength := NumGet(addr+0, "UShort"), addrLimit := addr + wLength, addr += 2
		,wValueLength := NumGet(addr+0, "UShort"), addr += 2
		,wType := NumGet(addr+0, "UShort"), addr += 2
		,szKey := StrGet(addr), addr += 2*(StrLen(szKey)+1), addr := (addr+3)&~3
		,ObjSetCapacity(this, "Data", size := wValueLength*(wType+1))
		,this.Name := szKey
		,this.DataSize := wValueLength
		,this.IsText := wType
		,DllCall("msvcrt\memcpy", "ptr", this.GetDataAddr(), "ptr", addr, "ptr", size, "cdecl"), addr += size, addr := (addr+3)&~3
		; if wType
			; ObjSetCapacity(this, "Data", -1)
		while addr < addrLimit
		{
			size := (NumGet(addr+0, "UShort") + 3) & ~3
			,this.Children.Push(new VersionRes(addr))
			,addr += size
		}
	}
	
	_NewEnum()
	{
		return this.Children._NewEnum()
	}
	
	AddChild(node)
	{
		this.Children.Push(node)
	}
	
	GetChild(name)
	{
		for k,v in this
			if v.Name = name
				return v
	}
	
	GetText()
	{
		if this.IsText
			return this.Data
	}
	
	SetText(txt)
	{
		this.Data := txt
		,this.IsText := true
		,this.DataSize := StrLen(txt)+1
	}
	
	GetDataAddr()
	{
		return ObjGetAddress(this, "Data")
	}
	
	Save(addr)
	{
		orgAddr := addr
		,addr += 2
		,NumPut(ds:=this.DataSize, addr+0, "UShort"), addr += 2
		,NumPut(it:=this.IsText, addr+0, "UShort"), addr += 2
		,addr += 2*StrPut(this.Name, addr+0, "UTF-16")
		,addr := (addr+3)&~3
		,realSize := ds*(it+1)
		,DllCall("msvcrt\memcpy", "ptr", addr, "ptr", this.GetDataAddr(), "ptr", realSize, "cdecl"), addr += realSize
		,addr := (addr+3)&~3
		for k,v in this
			addr += v.Save(addr)
		size := addr - orgAddr
		,NumPut(size, orgAddr+0, "UShort")
		return size
	}
}

ChangeVersionInfo(ExeFile, hUpdate, verInfo)
{
	hModule := LoadLibraryEx(ExeFile, 0, 2)
	if !hModule
		return MsgBox("Error: Error opening destination file.")
	
	hRsrc := FindResource(hModule, 1, 16) ; Version Info\1
	hMem := LoadResource(hModule, hRsrc)
	vi := new VersionRes(LockResource(hMem))
	FreeLibrary(hModule)
	
	ffi := vi.GetDataAddr()
	props := SafeGetViChild(SafeGetViChild(vi, "StringFileInfo"), "040904b0")
	for k,v in verInfo
	{
		if IsLabel(lbl := "_VerInfo_" k)
			gosub %lbl%
		continue
		_VerInfo_Name:
		SafeGetViChild(props, "ProductName").SetText(v)
		SafeGetViChild(props, "InternalName").SetText(v)
		return
		_VerInfo_Description:
		SafeGetViChild(props, "FileDescription").SetText(v)
		return
		_VerInfo_Version:
		SafeGetViChild(props, "FileVersion").SetText(v)
		SafeGetViChild(props, "ProductVersion").SetText(v)
		ver := VersionTextToNumber(v)
		hiPart := (ver >> 32)&0xFFFFFFFF, loPart := ver & 0xFFFFFFFF
		NumPut(hiPart, ffi+8, "UInt"), NumPut(loPart, ffi+12, "UInt")
		NumPut(hiPart, ffi+16, "UInt"), NumPut(loPart, ffi+20, "UInt")
		return
		_VerInfo_Copyright:
		SafeGetViChild(props, "LegalCopyright").SetText(v)
		return
		_VerInfo_OrigFilename:
		SafeGetViChild(props, "OriginalFilename").SetText(v)
		return
		_VerInfo_CompanyName:
		SafeGetViChild(props, "CompanyName").SetText(v)
		return
	}
	
	VarSetCapacity(newVI, 16384) ; Should be enough
	viSize := vi.Save(&newVI)
	if !UpdateResource(hUpdate, 16, 1, 0x409, &newVI, viSize)
		return MsgBox("Error changing the version information.")
}

VersionTextToNumber(v)
{
	r := 0, i := 0
	while i < 4 && RegExMatch(v, "^(\d+).?", o)
	{
		v:= Substr(v,o.Len + 1)
		val := o[1] + 0
		r |= (val&0xFFFF) << ((3-i)*16)
		i++
	}
	return r
}

SafeGetViChild(vi, name)
{
	c := vi.GetChild(name)
	if !c
	{
		c := new VersionRes()
		c.Name := name
		vi.AddChild(c)
	}
	return c
}
;~ #DllImport,FindResource,FindResourceW,PTR,0,STR,,STR,,PTR
;~ #DllImport,BeginUpdateResource,BeginUpdateResourceW,STR,,INT,0,PTR
;~ #DllImport,UpdateResource,UpdateResource,PTR,0,STR,,STR,,USHORT,0,PTR,0,UINT,0
;~ #DllImport,EndUpdateResource,EndUpdateResource,PTR,0,INT,0



/*
r:=Resource()
If (InStr(A_AhkVersion,"1")=1) {
  MsgBox Requires AutoHotkey V2 version, script will exit now!
  ExitApp
}

;~ SetWorkingDir,% A_ScriptDir
SetWorkingDir,C:\Scratch\Program Files\AutoHotKey\
;~ RCData:={("AutoHotkey 2\Win32w"):["AutoHotkey.dll","AutoHotkeyMini.dll"],("AutoHotkey 2\x64w"):["AutoHotkey.dll","AutoHotkeyMini.dll"]
      ;~ ,("AutoHotkey 1\Win32w"):["AutoHotkey.dll","AutoHotkeyMini.dll"],("AutoHotkey 1\Win32a"):["AutoHotkey.dll","AutoHotkeyMini.dll"]
      ;~ ,("AutoHotkey 1\x64w"):["AutoHotkey.dll","AutoHotkeyMini.dll"]}
RCData:={("AutoHotkey 2\Win32w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"],("Autohotkey 2\x64w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"]}
;~ RCData:={("AutoHotkey 2\Win32w"):["AutoHotkey.dll","AutoHotkeyMini.dll"]}
;~ RCData:={("AutoHotkey 2\x64w"):["AutoHotkey.dll","AutoHotkeyMini.dll"]}

for k,o in RCData
{
  ;~ If A_Index=1
    ;~ continue
  sourcedir:=A_ScriptDir "\" k "\"
  exe:=sourcedir "AutoHotkey.exe"
  hUpdate:=r.begin(exe)

  ;~ ResourceDelete(exe,10,"MSVCR100.DLL",1033)
  ;~ ListVars
  ;~ var:=FileRead("*c " A_ScriptDir "\MSVCR100" (k="x64w"?64:32) ".zip")
  ;~ ResourceUpdate(var,FileGetSize(A_ScriptDir "\MSVCR100" (k="x64w"?64:32) ".zip"),exe,10,"MSVCR100.DLL",1033)
  for k,v in o
  {
    FileDelete,% A_ScriptDir "\" v ".zip"
    FileDelete,% A_ScriptDir "\" v
    FileCopy,% sourcedir "\" v,% v,1
    RunWait mpress.exe %v%,,HIDE
    Sleep 1000
    ZipFileRaw(A_ScriptDir "\" v,A_ScriptDir "\" v ".zip") ;,"AutoHotkey")
    var:=FileRead("*c " A_ScriptDir "\" v ".zip")
    if r.find(hUpdate,10,StrUpper(v))
      If !r.update(hUpdate,10,StrUpper(v),1033,0,0)
        MsgBox % "Delete: " StrUpper(v) "-" ErrMsg()
    If !r.update(hUpdate,10,StrUpper(v),1033,&var,FileGetSize(A_ScriptDir "\" v ".zip"))
      MsgBox % "Update: " StrUpper(v) "-" ErrMsg()
    FileDelete,% A_ScriptDir "\" v
    FileDelete,% A_ScriptDir "\" v ".zip"
  }
  LoopFiles,% A_ScriptDir "\" SubStr(k,1,InStr(k,"\")-1) "\reslib\*.ahk"
  {
    FileDelete,% A_ScriptDir "\" A_LoopFileName ".zip"
    ZipFileRaw(A_ScriptDir "\" SubStr(k,1,InStr(k,"\")-1) "\reslib\" A_LoopFileName,A_ScriptDir "\" A_LoopFileName ".zip") ;,"AutoHotkey")
    var:=FileRead("*c " A_ScriptDir "\" A_LoopFileName ".zip")
    if r.find(hUpdate,"LIB",StrUpper(A_LoopFileName))
      If !r.update(hUpdate,"LIB",StrUpper(A_LoopFileName),1033,0,0)
        MsgBox % "Delete: " StrUpper(A_LoopFileName) "-" ErrMsg()
    If !r.update(hUpdate,"LIB",StrUpper(A_LoopFileName),1033,&var,FileGetSize(A_ScriptDir "\" A_LoopFileName ".zip"))
      MsgBox % "Update: " StrUpper(A_LoopFileName) "-" ErrMsg()
    FileDelete,% A_ScriptDir "\" A_LoopFileName ".zip"
  }
  If !r.end(hUpdate,0)
    MsgBox End: %ErrMsg()%
}
MsgBox Finished
;~ lib:="C:\Scratch\Program Files\AutoHotkey\AutoHotkey 1\lib\"
;~ LibRes:=["DllThread.ahk","MiniThread.ahk"]
;~ for k,v in LibRes
;~ {
	;~ size:=VarZ_Load(dll,lib v)
	;~ If !ressize:=VarZ_Compress(dll,size)
		;~ VarZ_Load(dll,lib v)
	;~ else size:=ressize
	;~ ResourceDelete(exe,"LIB",v,1033),ResourceUpdate(dll,size,exe,"LIB",v,1033)
;~ }



Resource(){
static r:={find:DynaCall("FindResourceW","t=tss"),lib:DynaCall("LoadLibraryW","t=w"),free:DynaCall("FreeLibrary","t")
,move:DynaCall("RtlMoveMemory","ttt"),load:DynaCall("LoadResource","t=tt"),lock:DynaCall("LockResource","t=t")
,size:DynaCall("SizeofResource","ui=tt"),begin:DynaCall("BeginUpdateResourceW","t=wi")
,update:DynaCall("UpdateResource","tssuhtui"),end:DynaCall("EndUpdateResource","ti")}
return r
}
ResourceReadData(ByRef data,dll,section,key,freeLibrary:=1){
static r:=Resource()
pdata:=r.lock(hResData:=r.load(hModule,hResource:=r.find(hModule:=r.lib(dll),section,key)))
,VarsetCapacity(data,SizeofResource:=r.size(hModule,hResource))
,r.move(&data,pData,SizeofResource)
if freeLibrary
r.free(hModule)
return SizeofResource
}
ResourceCreateDll(F:="empty.dll") {
static CreateFile:=DynaCall("CreateFile","ui=tuitttuiuiuiuitui","",0x40000000,2,0,2),write:=DynaCall("WriteFile","ttuiui*t")
,CloseHandle:=DynaCall("CloseHandle","t"),Trg,init:=VarSetCapacity(Trg,1536,0)+Numput(TS,Trg,192),TS:=DateAdd(A_NowUTC,1970,"s")
,Src:="0X5A4DY3CXB8YB8X4550YBCX2014CYCCX210E00E0YD0X7010BYD8X400YE4X1000YE8X1000YECX78AE0000YF0X1000YF4X200YF8X10005YFCX10005Y100X4Y108X3000Y10CX200Y114X2Y118X40000Y11CX2000Y120X100000Y124X1000Y12CX10Y140X1000Y144X10Y158X2000Y15CX8Y1B0X7273722EY1B4X63Y1B8X10Y1BCX1000Y1C0X200Y1C4X200Y1D4X40000040Y1D8X6C65722EY1DCX636FY1E0X8Y1E4X2000Y1E8X200Y1ECX400Y1FCX42000040"
LoopParse,%Src%,XY
Mod(A_Index,2)?O:="0x" A_LoopField:NumPut("0x" A_LoopField,Trg,O)
If 0<hF:=CreateFile[&F]
Write[&hF,&Trg,1536,w],CloseHandle[hF]
Return w=1536
}
ResourceExist(dll,section,key,freeLibrary:=1){
static r:=Resource()
return Exist:=r.find(hModule:=r.lib(dll),section,key)?1:0,r.free(hModule),Exist
}
ResourceUpdate(ByRef data,size,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin(dll){
result:=r.update(hUpdate,section,key,language,&data,size)
result:=r.end(hUpdate,!result)
} else result:=0
return result
}
ResourceDelete(dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin(dll){
result:=r.update(hUpdate,section,key,language,0,0)
result:=r.end(hUpdate,!result)
} else result:=0
return result
}
ResourceUpdateFile(File,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin(dll){
FileRead,Bin,*c %File%
FileGetSize,nSize,%File%
If nSize
result:=r.update[hUpdate,section,key,language,&Bin,nSize]
result:=r.end(hUpdate,!result)
} else result:=0
return result
}
ResourceUpdateFiles(Folder,dll,section,key,language:=0){
static r:=Resource()
If hUpdate:=r.begin(dll){
_result:=0
LoopFiles,% Folder "\*",F
{
FileRead,Bin,*c %A_LoopFileFullPath%
FileGetSize,nSize,%A_LoopFileFullPath%
If nSize
If result:=r.update(hUpdate,section,key,language,&Bin,nSize)
_result++
}
r.end(hUpdate,!_result)
} else _result:=0
return _result
}