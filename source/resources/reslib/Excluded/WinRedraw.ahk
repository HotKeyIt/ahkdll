WinRedraw(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
ErrorLevel:=!DllCall("InvalidateRect","PTR",aTitle aText=""?WinExist():WinExist(aTitle,aText,aExcludeTitle,aExcludeText),"PTR", 0,"Char",TRUE)
}