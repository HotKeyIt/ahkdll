FileCreateShortcut(aTargetFile, aShortcutFile, aWorkingDir:="", aArgs:="", aDescription:="", aIconFile:="", aHotkey:="", aIconNumber:="", aRunState:=""){
  static HOTKEYF_CONTROL:=2,HOTKEYF_ALT:=4
  bSucceeded := false
  if (psl := ComObjCreate("{00021401-0000-0000-C000-000000000046}","{000214F9-0000-0000-C000-000000000046}")){ ; CLSID_ShellLink / IID_IShellLink
		DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*20,"PTR"),"PTR",psl,"Str",aTargetFile) ; SetPath
		if aWorkingDir
			DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*9,"PTR"),"PTR",psl,"Str",aWorkingDir) ; SetWorkingDirectory
		if aArgs
			DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*11,"PTR"),"PTR",psl,"Str",aArgs) ; SetArguments
		if aDescription
			DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*7,"PTR"),"PTR",psl,"Str",aDescription) ; SetDescription
		if aIconFile ; Doesn't seem necessary to validate aIconNumber as not being negative, etc.
			DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*17,"PTR"),"PTR",psl,"Str",aIconFile,"Int",aIconNumber ? aIconNumber-1 : 0) ; SetIconLocation
    if aHotkey
		{
			; If badly formatted, it's not a critical error, just continue.
			; Currently, only shortcuts with a CTRL+ALT are supported.
			; AutoIt3 note: Make sure that CTRL+ALT is selected (otherwise invalid)
			vk := GetKeyVK(aHotkey)
			if (vk)
				; Vk in low 8 bits, mods in high 8:
				DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*13,"PTR"),"PTR",psl,"Int",vk | ((HOTKEYF_CONTROL | HOTKEYF_ALT) << 8) ) ;SetHotkey
		}
		if aRunState ; No validation is done since there's a chance other numbers might be valid now or in the future.
			DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*15,"PTR"),"PTR",psl,"Int",aRunState+0) ; SetShowCmd

		if (ppf := ComObjQuery(psl,"{0000010b-0000-0000-C000-000000000046}")){ ;IID_IPersistFile
			if (0>=DllCall(NumGet(NumGet(ppf,"PTR")+A_PtrSize*6,"PTR"),"PTR",ppf,"Str",aShortcutFile,"Int",true)) ;(ppf->Save(aShortcutFile, TRUE)))
				ErrorLevel:=0,bSucceeded := true ; Indicate success.
			DllCall(NumGet(NumGet(ppf,"PTR")+A_PtrSize*2,"PTR"),"PTR",ppf) ;ppf->Release()
		}
		DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*2,"PTR"),"PTR",psl) ;psl->Release()
	}
}
