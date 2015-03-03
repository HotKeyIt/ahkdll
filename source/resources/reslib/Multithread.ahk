multithread(script:="",param:="",IsFile:=0,dll:="AutoHotkey.dll"){
  static object,base:={__Delete:"multithread"},functions:="ahkFunction:s=sssssssssss|ahkPostFunction:i=sssssssssss|ahkdll:ut=sss|ahktextdll:ut=sss|ahkReady:|ahkReload:i|ahkTerminate:i=i|addFile:ut=sucuc|addScript:ut=si|ahkExec:ui=s|ahkassign:ui=ss|ahkExecuteLine:ut=utuiui|ahkFindFunc:ut=s|ahkFindLabel:ut=s|ahkgetvar:s=sui|ahkLabel:ui=sui|ahkPause:s|ahkIsUnicode:)"
  If IsObject(script){
    script.ahkterminate(script.timeout?script.timeout:0),MemoryFreeLibrary(script[""])
    return
  }
  object:=IsObject(obj)?obj:{},object[""]:=ResourceLoadLibrary(dll)
  If !object[""]
    object[""]:=MemoryLoadLibrary(dll)
  Loop,Parse,functions,|
  {
    StringSplit,v,A_LoopField,:
    object[v1]:=DynaCall(MemoryGetProcAddress(object[""],v1),v2)
  }
  object.base:=base
  If !(script+0!="" || script=0)
    object[IsFile?"ahkdll":"ahktextdll"](script,"",param)
  return object
}