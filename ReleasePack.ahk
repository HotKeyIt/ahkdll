; Embeds AutoHotkey.dll, AutoHotkeyMini.dll, 7-zip and (optionally) reslib\*.ahk into an AutoHotkey.exe or
; AutoHotkeySC.bin, with the same resource names and compression as CleanUpAndPack.ahk. Used by MakeRelease.ps1.
; Runs with AutoHotkey_H v1 (CleanUpAndPack.ahk refuses to run in v1).
; Usage: AutoHotkey.exe ReleasePack.ahk <target> <AutoHotkey.dll> <AutoHotkeyMini.dll> <7-zip dll> [<reslib dir>]
#NoEnv
#NoTrayIcon
target := A_Args[1], dll := A_Args[2], mini := A_Args[3], sevenzip := A_Args[4], reslib := A_Args[5]
h := Begin(target)
AddZipped(h, 10, "F903E44B8A904483A1732BA84EA6191F", dll)
AddZipped(h, 10, "FC2328B39C194A4788051A3B01B1E7D5", mini)
AddZipped(h, 10, "556EA2A65AE54D58BC52C792B3ED2ED0", sevenzip)
if (reslib != "")
	Loop, Files, %reslib%\*.ahk
	{
		StringUpper, name, A_LoopFileName
		AddZipped(h, "LIB", name, A_LoopFileFullPath)
	}
End(h)
ExitApp 0

Begin(file) {
	if !FileExist(file)
		Fail("missing " file)
	if !h := DllCall("BeginUpdateResource", "Str", file, "Int", 0, "Ptr")
		Fail("BeginUpdateResource " file)
	return h
}
End(h) {
	if !DllCall("EndUpdateResource", "Ptr", h, "Int", 0)
		Fail("EndUpdateResource")
}
AddZipped(h, type, name, file) {
	if !FileExist(file)
		Fail("missing " file)
	FileRead, data, *c %file%
	FileGetSize, sz, %file%
	zsz := ZipRawMemory(&data, sz, zipped)
	if !zsz
		Fail("ZipRawMemory " file)
	if type is integer
		ok := DllCall("UpdateResource", "Ptr", h, "Ptr", type, "Str", name, "UShort", 1033, "Ptr", &zipped, "UInt", zsz)
	else
		ok := DllCall("UpdateResource", "Ptr", h, "Str", type, "Str", name, "UShort", 1033, "Ptr", &zipped, "UInt", zsz)
	if !ok
		Fail("UpdateResource " name)
}
Fail(msg) {
	FileAppend, PACK ERROR: %msg% (%A_LastError%)`n, *
	ExitApp 1
}
