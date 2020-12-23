ResourceIndexToId(aModule, aType, aIndex){
	static enum_data:=Struct("find_index,index,result") ;ResourceIndexToIdEnumData
		,ResourceIndexToIdEnumProc:=CallbackCreate("ResourceIndexToIdEnumProc","",4),RT_GROUP_ICON := 3 + 11
	enum_data.find_index := aIndex
	enum_data.index := 0
	enum_data.result := -1 ; Return value of -1 indicates failure, since ID 0 may be valid.
	DllCall("EnumResourceNames","PTR",aModule,"PTR", aType,"PTR", ResourceIndexToIdEnumProc, "PTR", enum_data[])
	return enum_data.result
}
ResourceIndexToIdEnumProc(hModule, lpszType, lpszName, lParam){
	static enum_data:=Struct("find_index,index,result") ;ResourceIndexToIdEnumData
	enum_data[] := lParam
	if (++enum_data.index = enum_data.find_index)
	{
		enum_data.result := lpszName
		return FALSE ; Stop
	}
	return TRUE ; Continue
}