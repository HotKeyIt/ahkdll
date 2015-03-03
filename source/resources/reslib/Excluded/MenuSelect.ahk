MenuSelect(aTitle:="", aText:="",aMenu1:="",aMenu2:="",aMenu3:="",aMenu4:="",aMenu5:="",aMenu6:="",aMenu7:="",aExcludeTitle:="",aExcludeText:=""){
	; Set up a temporary array make it easier to traverse nested menus & submenus
	; in a loop.  Also add a NULL at the end to simplify the loop a little:
	;;LPTSTR menu_param[] = {aMenu1, aMenu2, aMenu3, aMenu4, aMenu5, aMenu6, aMenu7, NULL}
	static MENU_ITEM_IS_SUBMENU := 0xFFFFFFFF,menu_text_size:=2048,MF_BYPOSITION:=1024,WM_COMMAND:=273,MIIM_SUBMENU:=0x4
	if ((!target_window := WinExist(aTitle, aText, aExcludeTitle, aExcludeText))
		|| (!hMenu := DllCall("GetMenu","PTR",target_window,"PTR")) ; Window has no menu bar.
		|| (1>menu_item_count := DllCall("GetMenuItemCount","PTR",hMenu))){ ; Menu bar has no menus.
		ErrorLevel:=1
		Return
	}

	menu_id := MENU_ITEM_IS_SUBMENU
	VarSetCapacity(menu_text,menu_text_size)
  ;~ MENUITEMINFO :="UINT cbSize;UINT fMask;UINT fType;UINT fState;UINT wID;HMENU hSubMenu;HBITMAP hbmpChecked;HBITMAP hbmpUnchecked;ULONG_PTR dwItemData;LPTSTR dwTypeData;UINT cch;HBITMAP hbmpItem"
  ;~ mi:=Struct(MENUITEMINFO,{cbSize:sizeof(MENUITEMINFO),fMask:MIIM_SUBMENU})
	;~ bool match_found;
	;~ size_t this_menu_param_length, menu_text_length;
	;~ int pos, target_menu_pos;
	;~ LPTSTR this_menu_param;
	While aMenu%A_Index%!="" {
		this_menu_param := aMenu%A_Index% ; For performance and convenience.
		if (!hMenu){  ; The nesting of submenus ended prior to the end of the list of menu search terms.
			ErrorLevel:=1
			return
		}
		this_menu_param_length := StrLen(this_menu_param)
		target_menu_pos := (SubStr(this_menu_param,-1) = "&") ? SubStr(this_menu_param,1,-1) - 1 : -1
		if (target_menu_pos > -1)
		{
			if (target_menu_pos >= menu_item_count){  ; Invalid menu position (doesn't exist).
				ErrorLevel:=1
				return
			}
			if (MENU_ITEM_IS_SUBMENU =  menu_id := DllCall("GetMenuItemID","PTR",hMenu,"PTR", target_menu_pos)){
				hMenu := DllCall("GetSubMenu","PTR",hMenu,"UInt", target_menu_pos,"PTR")
				DllCall("DrawMenuBar","PTR",hMenu)
				menu_item_count := DllCall("GetMenuItemCount","PTR",hMenu)
			} else
				menu_item_count := 0,hMenu := 0
		}
		else ; Searching by text rather than numerical position.
		{
			match_found := false
			Loop % menu_item_count {
				menu_text_length := DllCall("GetMenuString","PTR",hMenu,"UInt", A_Index-1,"PTR", &menu_text,"UInt", menu_text_size-1,"UInt",MF_BYPOSITION)
				VarSetCapacity(menu_text,-1)
				; v1.0.43.03: It's debatable, but it seems best to support locale's case insensitivity for
				; menu items, since menu names tend to adapt to the user's locale.  By contrast, things
				; like process names (in the Process command) do not tend to change, so it seems best to
				; have them continue to use stricmp(): 1) avoids breaking existing scripts; 2) provides
				; consistent behavior across multiple locales; 3) performance.
				if (!match_found := InStr(menu_text,this_menu_param))
				{
					StrReplace,menu_text2,%menu_text%,&
					match_found := InStr(menu_text2,this_menu_param)
				}
				if (match_found)
				{
					if (MENU_ITEM_IS_SUBMENU ==  menu_id := DllCall("GetMenuItemID","PTR",hMenu,"PTR", A_Index-1,"UINT")){
						hMenu := DllCall("GetSubMenu","PTR",hMenu,"UInt", A_Index-1,"PTR")
						DllCall("SetForegroundWindow","PTR",WinExist(aTitle)+0)
						DllCall("TrackPopupMenuEx","PTR",hMenu,"UInt", 0,"UInt", 100,"UInt", 100,"PTR", WinExist(aTitle)+0,"PTR", 0)
						DllCall("SetForegroundWindow","PTR",WinExist(aTitle)+0)
						Sleep 100
            menu_item_count := DllCall("GetMenuItemCount","PTR",hMenu)
					} else
						menu_item_count := 0,hMenu := 0
					break
				}
			} ; inner for()
			if (!match_found){ ; The search hierarchy (nested menus) specified in the params could not be found.
				ErrorLevel:=1
				return
			}
		} ; else
	} ; outer for()

	; This would happen if the outer loop above had zero iterations due to aMenu1 being NULL or blank,
	; or if the caller specified a submenu as the target (which doesn't seem valid since an app would
	; next expect to ever receive a message for a submenu?):
	if (menu_id == MENU_ITEM_IS_SUBMENU){
		ErrorLevel:=1
		return
	}

	; Since the above didn't return, the specified search hierarchy was completely found.
	PostMessage,%WM_COMMAND%, %menu_id%,,,%aTitle%,%aText%,%aExcludeTitle%,%aExcludeText%
	return ; Indicate success.

}
