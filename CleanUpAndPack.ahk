If (InStr(A_AhkVersion,"1")=1) {
  MsgBox Requires AutoHotkey V2 version, script will exit now!
  ExitApp
}

SetWorkingDir,% A_ScriptDir

dirs:=[A_ScriptDir "\bin"]
subs:={"Win32w":1,"x64w":1,"Win32a":1}
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

RCData:={("bin\Win32a"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"],("bin\Win32w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"],("bin\x64w"):["AUTOHOTKEY.DLL","AUTOHOTKEYMINI.DLL"]}

for k,v in RCData
  LoopFiles % A_ScriptDir "\" k "\*.dll"
  {
    if !hUpdate:=BeginUpdateResource(A_LoopFileFullPath)
    {
      MsgBox % "Error Begin: " A_LoopFileFullPath "`n" ErrMsg()
      ExitApp
    }
    ChangeVersionInfo(A_LoopFileFullPath, hUpdate, {version: version "-H" H_version})
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
      FileDelete,% A_ScriptDir "\temp\" v
      FileDelete,% A_ScriptDir "\temp\" v ".zip"
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
      MsgBox End: %ErrMsg()%
  }
}
MsgBox Finished