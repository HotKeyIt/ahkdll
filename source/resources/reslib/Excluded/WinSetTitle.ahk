WinSetTitles(aTitle:="",aText:="",nTitle:="",aExcludeTitle:="",aExcludeText:=""){
  If IsObject(aTitle){
    ErrorLevel:=0
    Loop aTitle.Length()
      ErrorLevel+=!DllCall("SetWindowText","PTR",WinExist(aTitle[A_Index]*),"Str",IsObject(nTitle)?nTitle[A_Index]:nTitle)
  } else
    ErrorLevel:=!DllCall("SetWindowText","PTR",aTitle nTitle=""?WinExist():WinExist(aTitle,aText,aExcludeTitle,aExcludeText),"Str",aText nTitle=""?aTitle:nTitle)
}