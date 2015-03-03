SysGet(aCmd, aValue:=0){
; Thanks to Gregory F. Hogg of Hogg's Software for providing sample code on which this function
; is based.
	; For simplicity and array look-up performance, this is done even for sub-commands that output to an array:
	static RECT := "left,top,right,bottom",MonitorInfoPackage := "int count;int monitor_number_to_find;MONITORINFOEX monitor_info_ex"
        ,CCHDEVICENAME := 32
        ,MONITORINFOEX := "DWORD cbSize;_RECT rcMonitor;_RECT rcWork;DWORD dwFlags;TCHAR szDevice[" CCHDEVICENAME "]"
        ,SYSGET_CMD_INVALID:=0, SYSGET_CMD_METRICS:=1, SYSGET_CMD_MONITORCOUNT:=2, SYSGET_CMD_MONITORPRIMARY:=3
												, SYSGET_CMD_MONITORAREA:=4, SYSGET_CMD_MONITORWORKAREA:=5, SYSGET_CMD_MONITORNAME:=6
				,SysGet_EnumMonitorProc:=RegisterCallback("SysGet_EnumMonitorProc","Fast",4),mip := Struct(MonitorInfoPackage)
  if (aCmd="") 
    cmd := SYSGET_CMD_INVALID
  else if (aCmd+0!="") 
    cmd := SYSGET_CMD_METRICS
  else if InStr(".MonitorCount.MonitorPrimary.Monitor.MonitorWorkArea.MonitorName.","." aCmd ".")
      cmd := SYSGET_CMD_%aCmd% ; Called "Monitor" vs. "MonitorArea" to make it easier to remember.
  else cmd := SYSGET_CMD_INVALID

	; Since command names are validated at load-time, this only happens if the command name
	; was contained in a variable reference.  But for simplicity of design here, return
	; failure in this case (unlike other functions similar to this one):
	if (cmd = SYSGET_CMD_INVALID)
		return DllCall("MessageBox","PTR",0,"Str","Invalid option " aCmd,"Str","Error","UInt",0) ? 1 : 1

	mip.Fill()  ; Improves maintainability to initialize unconditionally, here.
	mip.monitor_info_ex.cbSize := sizeof(MONITORINFOEX) ; Also improves maintainability.

	; EnumDisplayMonitors() must be dynamically loaded; otherwise, the app won't launch at all on Win95/NT.
	;typedef BOOL (WINAPI* EnumDisplayMonitorsType)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
	;static EnumDisplayMonitorsType MyEnumDisplayMonitors = (EnumDisplayMonitorsType)
	;	GetProcAddress(GetModuleHandle(_T("user32")), "EnumDisplayMonitors");

	If (cmd = SYSGET_CMD_METRICS) ; In this case, aCmd is the value itself.
			return DllCall("GetSystemMetrics","Int",aCmd)  ; Input and output are both signed integers.

	; For the next few cases, I'm not sure if it is possible to have zero monitors.  Obviously it's possible
	; to not have a monitor turned on or not connected at all.  But it seems likely that these various API
	; functions will provide a "default monitor" in the absence of a physical monitor connected to the
	; system.  To be safe, all of the below will assume that zero is possible, at least on some OSes or
	; under some conditions.  However, on Win95/NT, "1" is assumed since there is probably no way to tell
	; for sure if there are zero monitors except via GetSystemMetrics(SM_CMONITORS), which is a different
	; animal as described below.
	else if (cmd = SYSGET_CMD_MONITORCOUNT){
		; Don't use GetSystemMetrics(SM_CMONITORS) because of this:
		; MSDN: "GetSystemMetrics(SM_CMONITORS) counts only display monitors. This is different from
		; EnumDisplayMonitors, which enumerates display monitors and also non-display pseudo-monitors."
		mip.monitor_number_to_find := COUNT_ALL_MONITORS
		;~ MsgBox % mip.monitor_number_to_find
		DllCall("EnumDisplayMonitors","PTR",0,"PTR", 0,"PTR", SysGet_EnumMonitorProc,"PTR", mip[])
		return mip.count ; Will assign zero if the API ever returns a legitimate zero.
	}
	; Even if the first monitor to be retrieved by the EnumProc is always the primary (which is doubtful
	; since there's no mention of this in the MSDN docs) it seems best to have this sub-cmd in case that
	; policy ever changes:
	else if (cmd = SYSGET_CMD_MONITORPRIMARY){
		DllCall("EnumDisplayMonitors","PTR",0,"PTR", 0,"PTR", SysGet_EnumMonitorProc,"PTR", mip[])
		return mip.count ; Will assign zero if the API ever returns a legitimate zero.
	}
	else if (cmd = SYSGET_CMD_MONITORAREA || cmd = SYSGET_CMD_MONITORWORKAREA){
		;~ Var *output_var_left, *output_var_top, *output_var_right, *output_var_bottom;
		; Make it longer than max var name so that FindOrAddVar() will be able to spot and report
		; var names that are too long:
		;~ TCHAR var_name[MAX_VAR_NAME_LENGTH + 20];
		; To help performance (in case the linked list of variables is huge), tell FindOrAddVar where
		; to start the search.  Use the base array name rather than the preceding element because,
		; for example, Array19 is alphabetically less than Array2, so we can't rely on the
		; numerical ordering:
		monitor_rect := Struct(RECT)
		mip.monitor_number_to_find := aValue  ; If this returns 0, it will default to the primary monitor.
		DllCall("EnumDisplayMonitors","PTR",0,"PTR", 0,"PTR", SysGet_EnumMonitorProc,"PTR", mip[])
		if (!mip.count || (mip.monitor_number_to_find && mip.monitor_number_to_find != mip.count))
		{
			; With the exception of the caller having specified a non-existent monitor number, all of
			; the ways the above can happen are probably impossible in practice.  Make all the variables
			; blank vs. zero to indicate the problem.
			return
		}
		; Otherwise:
		DllCall("RtlMoveMemory","PTR",monitor_rect[],"PTR",(cmd = SYSGET_CMD_MONITORAREA) ? mip.monitor_info_ex.rcMonitor[""] : mip.monitor_info_ex.rcWork[""],"PTR",sizeof(monitor_rect))
		return monitor_rect
	}
	else if (cmd = SYSGET_CMD_MONITORNAME){
		mip.monitor_number_to_find := aValue  ; If this returns 0, it will default to the primary monitor.
		DllCall("EnumDisplayMonitors","PTR",0,"PTR", 0,"PTR", SysGet_EnumMonitorProc,"PTR", mip[])
		if (!mip.count || (mip.monitor_number_to_find && mip.monitor_number_to_find != mip.count))
			; With the exception of the caller having specified a non-existent monitor number, all of
			; the ways the above can happen are probably impossible in practice.  Make the variable
			; blank to indicate the problem:
			return
		else
			return StrGet(mip.monitor_info_ex.szDevice[""])
	}
}

SysGet_EnumMonitorProc(hMonitor, hdcMonitor, lprcMonitor, lParam){
	static COUNT_ALL_MONITORS := -2147483647 - 1,MONITORINFOF_PRIMARY:=1
        ,MonitorInfoPackage := "int count;int monitor_number_to_find;MONITORINFOEX monitor_info_ex"
        ,mip := Struct(MonitorInfoPackage) ;For performance and convenience.
	mip[]:=lParam
	if (mip.monitor_number_to_find = COUNT_ALL_MONITORS)
	{
		++mip.count
		return TRUE  ; Enumerate all monitors so that they can be counted.
	}
	; GetMonitorInfo() must be dynamically loaded otherwise, the app won't launch at all on Win95/NT.
	;~ typedef BOOL (WINAPI* GetMonitorInfoType)(HMONITOR, LPMONITORINFO)
	;~ static GetMonitorInfoType MyGetMonitorInfo = (GetMonitorInfoType)
		;~ GetProcAddress(GetModuleHandle(_T("user32")), "GetMonitorInfo" WINAPI_SUFFIX)
	if !DllCall("GetMonitorInfoW","PTR",hMonitor, "PTR",mip.monitor_info_ex[""]) ; Failed.  Probably very rare.
		return FALSE ; Due to the complexity of needing to stop at the correct monitor number, do not continue.
		; In the unlikely event that the above fails when the caller wanted us to find the primary
		; monitor, the caller will think the primary is the previously found monitor (if any).
		; This is just documented here as a known limitation since this combination of circumstances
		; is probably impossible.
	++mip.count ; So that caller can detect failure, increment only now that failure conditions have been checked.
	if (mip.monitor_number_to_find) ; Caller gave a specific monitor number, so don't search for the primary monitor.
	{
		if (mip.count = mip.monitor_number_to_find) ; Since the desired monitor has been found, must not continue.
			return FALSE
	}
	else ; Caller wants the primary monitor found.
		; MSDN docs are unclear that MONITORINFOF_PRIMARY is a bitwise value, but the name "dwFlags" implies so:
		if (mip.monitor_info_ex.dwFlags & MONITORINFOF_PRIMARY)
			return FALSE  ; Primary has been found and "count" contains its number. Must not continue the enumeration.
			; Above assumes that it is impossible to not have a primary monitor in a system that has at least
			; one monitor.  MSDN certainly implies this through multiple references to the primary monitor.
	; Otherwise, continue the enumeration:
	return TRUE
}
