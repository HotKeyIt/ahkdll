IsDifferentVolumes(szPath1, szPath2){
	static szP1Drive,szP2Drive,i:=VarSetCapacity(szP1Drive,3+1) VarSetCapacity(szP2Drive,3+1)
	; Get full pathnames
	szP1:=GetFullPathName(szPath1),szP2:=GetFullPathName(szPath2)
	; Split the target into bits
	,DllCall("msvcrt\_wsplitpath","Str",szPath1,"Str",szP1Drive,"PTR",0,"PTR",0,"PTR",0,"Cdecl")
  ,DllCall("msvcrt\_wsplitpath","Str",szPath2,"Str",szP2Drive,"PTR",0,"PTR",0,"PTR",0,"Cdecl")
	if NumGet(szP1Drive,"Short") = 0 || NumGet(szP2Drive,"Short") = 0
		; One or both paths is a UNC - assume different volumes
		return true
	else
		return szP1Drive=szP2Drive
}

