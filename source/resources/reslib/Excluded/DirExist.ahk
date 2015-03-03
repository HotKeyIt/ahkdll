DirExist(szPath){
dwTemp := DllCall("GetFileAttributes","Str",szPath,"UInt")
return dwTemp != 0xffffffff && (dwTemp & 16) ;FILE_ATTRIBUTE_DIRECTORY
}
