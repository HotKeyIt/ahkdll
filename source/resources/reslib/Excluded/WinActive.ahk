WinActive(aTitle:="",aText:="",aExcludeTitle:="",aExcludeText:=""){
  return (aTitle aText=""?WinExist():WinExist(aTitle,aText,aExcludeTitle,aExcludeText))=DllCall("GetForegroundWindow","PTR")
}