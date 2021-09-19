CreateScript(script, path:="",pw:=""){
  static mScript:=""
  local Data2:=aScript:=""
  WorkingDir:=A_WorkingDir
  script:=StrReplace(StrReplace(script,"`n,`r`n"),"`r`r","`r")
  If RegExMatch(script,"m)^[^:]+:[^:]+|[a-zA-Z0-9#_@]+\{}$"){
    If !(mScript){
      If (A_IsCompiled){
        lib := GetModuleHandle()
        If !(res := FindResource(lib,"E4847ED08866458F8DD35F94B37001C0",10)){
          MsgBox("Could not extract script!")
          return
        }
        DataSize := SizeofResource(lib, res)
        ,hresdata := LoadResource(lib,res)
        ,pData := LockResource(hresdata),(Data2:=UnZipRawMemory(pData,DataSize,pw))?pData:=Data2.Ptr:""
        If (DataSize){
          mScript := StrReplace(StrReplace(StrReplace(StrReplace(StrGet(pData,"UTF-8"),"`n","`r`n"),"`r`r","`r"),"`r`r","`r"),"`n`n","`n")
          line:=Buffer(16384*2)
          Loop Parse, mScript,"`n","`r"
          {
            CryptStringToBinaryW(StrPtr(A_LoopField), 0, 0x1, line.Ptr, getvar(aSizeEncrypted:=16384*2), 0, 0)
            if (NumGet(line,"UInt") != 0x04034b50)
              break
            aScript .= StrGet(UnZipRawMemory(line.Ptr,aSizeEncrypted,pw),"UTF-8") "`r`n"
          }
          if aScript
            mScript:= "`r`n" aScript "`r`n"
          else mScript :="`r`n" mScript "`r`n"
        }
      } else {
        mScript:="`r`n" StrReplace(StrReplace(FileRead(path?path:A_ScriptFullPath),"`n","`r`n"),"`r`r","`r") "`r`n" 
        Loop Parse, mScript,"`n","`r"
        {
          If A_Index=1
            mScript:=""
          If RegExMatch(A_LoopField,"i)^\s*#include"){
            temp:=RegExReplace(A_LoopField,"i)^\s*#include[\s+|,]")
            If InStr(temp,"`%")
              Loop Parse, temp,"`%"
                If A_Index=1
                  temp:=A_LoopField
                else if !Mod(A_Index,2)
                  _temp:=A_LoopField
                else _temp:=%_temp%,temp.=_temp A_LoopField,_temp:=""
			If InStr(FileExist(trim(temp,"<>")),"D"){
				SetWorkingDir trim(temp,"<>")
				continue
			} else If InStr(FileExist(temp),"D"){
				SetWorkingDir temp
				continue
			} else If (SubStr(temp,1,1) . SubStr(temp,-1) = "<>"){
              If !FileExist(_temp:=A_ScriptDir "\lib\" trim(temp,"<>") ".ahk")
                If !FileExist(_temp:=A_MyDocuments "\AutoHotkey\lib\" trim(temp,"<>") ".ahk")
                  If !FileExist(_temp:=SubStr(A_AhkPath,1,InStr(A_AhkPath,"\",1,-1)) "lib\" trim(temp,"<>") ".ahk")
                    If FileGetShortcut(SubStr(A_AhkPath,1,InStr(A_AhkPath,"\",1,-1)) "lib.lnk",_temp)
                      _temp:=_temp "\" trim(temp,"<>") ".ahk"
				mScript.= FileRead(_temp) "`r`n"
			} else mScript.= FileRead(temp) "`r`n"
          } else mScript.=A_LoopField "`r`n"
        }
      }
    }
    Loop Parse, script,"`n","`r"
    {
      If A_Index=1
        script:=""
      else If A_LoopField=""
        Continue
      If (RegExMatch(A_LoopField,"^[^:\s]+:[^:\s=]+$")){
        label:=StrSplit(A_LoopField,":")
        If (label.Length=2) ; cannot test if global label exist inside function (out of scope): and IsLabel(label[1]) and IsLabel(label[2]))
          script .=SubStr(mScript
            , h:=InStr(mScript,"`r`n" label[1] ":`r`n")
            , InStr(mScript,"`r`n" label[2] ":`r`n")-h) . "`r`n"
      } else if RegExMatch(A_LoopField,"^[^\{}\s]+\{}$")
        label := SubStr(A_LoopField,1,-2),script .= SubStr(mScript
          , h:=RegExMatch(mScript,"i)\n" label "\([^\\)\n]*\)\n?\s*\{")
          , RegExMatch(mScript,"\n}\s*\n\K",,h)-h) . "`r`n"
      else script .= A_LoopField "`r`n"
    }
  }
  script:=StrReplace(script,"`r`n","`n")
  SetWorkingDir WorkingDir
  Return Script
}