AhkExported(mapping:=""){
  static init
  static functions:="ahkFunction:s==sssssssssss|ahkPostFunction:s==sssssssssss|ahkExec:ui==s|ahkassign:ui==ss|ahkExecuteLine:t==tuiui|ahkFindFunc:t==s|ahkFindLabel:t==s|ahkgetvar:s==sui|ahkLabel:ui==sui|ahkPause:i==s"
  If !init&&init:=Object(){
    If !A_IsCompiled
      functions.="|addFile:t==si|addScript:t==si"
    VarSetCapacity(file,512),DllCall("GetModuleFileName","UInt",DllCall("GetModuleHandle","UInt",0),"Uint",&file,"UInt",520),DllCall("LoadLibrary","PTR",&file)
    LoopParse,%functions%,|
    { StrSplit,v,%A_LoopField%,:
      if mapping&&!name:=""{
        loopParse,%Mapping%,%A_Space%
          If SubStr(A_LoopField,1,InStr(A_LoopField,"=")-1)=v.1
            name:=SubStr(A_LoopField,InStr(A_LoopField,"=")+1)
          else if A_LoopField=v.1
            name:=A_LoopField
        if name&&!init[name]
          init[name]:=DynaCall((A_IsCompiled?A_ScriptFullPath:A_AhkPath) "\" v.1,v.2)
        continue
      }else name:=v.1
      init[name]:=DynaCall((A_IsCompiled?A_ScriptFullPath:A_AhkPath) "\" v.1,v.2)
    }
  }
  return init
}