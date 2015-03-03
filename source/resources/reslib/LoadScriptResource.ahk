LoadScriptResource(Name, ByRef DataSize := 0, Type := 10)
{
    res := DllCall("FindResource",ptr, lib := DllCall("GetModuleHandle",ptr, 0, "ptr"), ptr, Name+0=""?&Name:Name,ptr, Type+0=""?&Type:Type, "ptr")
    DataSize := DllCall("SizeofResource",ptr, lib,ptr, res, "uint")
    return DllCall("LockResource",ptr, DllCall("LoadResource",ptr, lib,ptr, res, "ptr"), "ptr")
}
LoadScriptResource_AHK()
{
    If !(data:=LoadScriptResource(">AHK WITH ICON<",DataSize))
		&& !(data:=LoadScriptResource(">AUTOHOTKEY SCRIPT<",DataSize)) {
		MsgBox AutoHotkey Script could not be extracted!
		return
	}
	return StrGet(data,DataSize,"UTF-8")
}