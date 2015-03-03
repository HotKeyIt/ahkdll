FileGetShortcut(aShortcutFile,ByRef output_var_target:="",ByRef output_var_dir:="",ByRef output_var_arg:="",ByRef output_var_desc:="",ByRef output_var_icon:="",ByRef output_var_icon_idx:="",ByRef output_var_show_state:=""){ ; Credited to Holger <Holger.Kotsch at GMX de>
	; These might be omitted in the parameter list, so it's okay if 
	; they resolve to NULL.  Also, load-time validation has ensured
	; that these are valid output variables (e.g. not built-in vars).
	; Load-time validation has ensured that these are valid output variables (e.g. not built-in vars).
  static MAX_PATH:=260,SLGP_UNCPRIORITY:=2,IID_IShellLink := "{000214F9-0000-0000-C000-000000000046}"
				,CLSID_ShellLink := "{00021401-0000-0000-C000-000000000046}",IID_IPersistFile := "{0000010b-0000-0000-C000-000000000046}"

	; For consistency with the behavior of other commands, the output variables are initialized to blank
	; so that there is another way to detect failure:
	output_var_target:=output_var_dir:=output_var_arg:=output_var_desc:=output_var_icon:=output_var_icon_idx:=output_var_show_state:=""
	,bSucceeded := false

	if !FileExist(aShortcutFile){
		ErrorLevel:=1
		return
	}
  if (psl := ComObjCreate("{00021401-0000-0000-C000-000000000046}","{000214F9-0000-0000-C000-000000000046}")){ ; CLSID_ShellLink / IID_IShellLink
    if (ppf := ComObjQuery(psl,"{0000010b-0000-0000-C000-000000000046}")){ ;IID_IPersistFile
			if (0<=DllCall(NumGet(NumGet(ppf,"PTR")+A_PtrSize*5,"PTR"),"PTR",ppf,"Str",aShortcutFile,"Int", 0)){ ;Load
				VarSetCapacity(buf,MAX_PATH*2+2)
				if IsByRef(output_var_target){
					DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*3,"PTR"),"PTR",psl,"PTR",&buf,"Int", MAX_PATH,"PTR", 0,"Int", SLGP_UNCPRIORITY) ;GetPath
          ,VarSetCapacity(buf,-1),output_var_target:=buf
				}
				if IsByRef(output_var_dir)
				{
          DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*8,"PTR"),"PTR",psl,"PTR",&buf,"Int",MAX_PATH) ;GetWorkingDirectory
          ,VarSetCapacity(buf,-1),output_var_dir:=buf
				}
				if IsByRef(output_var_arg)
				{
          DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*10,"PTR"),"PTR",psl,"PTR",&buf,"Int",MAX_PATH) ;GetArguments
					,VarSetCapacity(buf,-1),output_var_arg:=buf
				}
				if IsByRef(output_var_desc)
				{
          DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*6,"PTR"),"PTR",psl,"PTR",&buf,"Int",MAX_PATH) ;GetDescription
          ,VarSetCapacity(buf,-1),output_var_desc:=buf ; Testing shows that the OS limits it to 260 characters.
				}
				if IsByRef(output_var_icon || output_var_icon_idx)
				{
          DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*16,"PTR"),"PTR",psl,"PTR",&buf,"Int",MAX_PATH,"PTR*",icon_index) ;GetIconLocation
					if IsByRef(output_var_icon)
						VarSetCapacity(buf,-1),output_var_icon:=buf
					if IsByRef(output_var_icon_idx)
						if (buf!="")
							output_var_icon_idx:=icon_index + 1  ; Convert from 0-based to 1-based for consistency with the Menu command, etc.
				}
				if IsByRef(output_var_show_state){
          DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*14,"PTR"),"PTR",psl,"PTR*",show_cmd) ;GetShowCmd
					,output_var_show_state:=show_cmd
					; For the above, decided not to translate them to Max/Min/Normal since other
					; show-state numbers might be supported in the future (or are already).  In other
					; words, this allows the flexibility to specify some number other than 1/3/7 when
					; creating the shortcut in case it happens to work.  Of course, that applies only
					; to FileCreateShortcut, not here.  But it's done here so that this command is
					; compatible with that one.
				}
				ErrorLevel:=0 ; Indicate success.
				,bSucceeded := true
			}
			DllCall(NumGet(NumGet(ppf,"PTR")+A_PtrSize*2,"PTR"),"PTR",ppf)
		}
		DllCall(NumGet(NumGet(psl,"PTR")+A_PtrSize*2,"PTR"),"PTR",psl)
	}
	If !bSucceeded
		ErrorLevel:=1
	return ; ErrorLevel might still indicate failure if one of the above calls failed.
}