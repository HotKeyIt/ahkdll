ahkthread_free(obj:=""){
	static objects:=[]
	if obj=""
    objects:=[]
	else if !IsObject(obj)
		return objects
	else If objects.HasKey(obj)
		objects.Remove(obj)
}
ahkthread(script:="",param:="",IsFile:=0,dll:="F903E44B8A904483A1732BA84EA6191F"){
  static base,ahkdll,functions,hRes:=DllCall("FindResource","PTR",0,"STR","F903E44B8A904483A1732BA84EA6191F","PTR",10,"PTR"),data:=LockResource(hResData:=LoadResource(0,hRes)),init1:=UnZipRawMemory(data,SizeofResource(0,hRes),ahkdll)
  if !base
    base:={__Delete:"ahkthread"},functions:="ahkFunction:s==sssssssssss|ahkPostFunction:i==sssssssssss|ahkdll:ut==ss|ahktextdll:ut==ss|ahkReady:|ahkReload:i==i|ahkTerminate:i==i|addFile:ut==sucuc|addScript:ut==si|ahkExec:ui==s|ahkassign:ui==ss|ahkExecuteLine:ut==utuiui|ahkFindFunc:ut==s|ahkFindLabel:ut==s|ahkgetvar:s==sui|ahkLabel:ui==sui|ahkPause:i==s|ahkIsUnicode:"
  If IsObject(script){
    script.ahkterminate(script.timeout?script.timeout:0),MemoryFreeLibrary(script[""])
    return
  }
  object:={(""):MemoryLoadLibrary(dll+0?dll:dll="F903E44B8A904483A1732BA84EA6191F"?&ahkdll:dll)}
  Loop,Parse,functions,|
  {
    v:=StrSplit(A_LoopField,":"),object[v.1]:=DynaCall(MemoryGetProcAddress(object[""],v.1),v.2)
	If (v.1="ahkFunction")
		object["_" v.1]:=DynaCall(MemoryGetProcAddress(object[""],v.1),"s==stttttttttt")
	else if (v.1="ahkPostFunction")
		object["_" v.1]:=DynaCall(MemoryGetProcAddress(object[""],v.1),"i==stttttttttt")
  }
  object.base:=base
  If !(script+0!="" || script=0)
    object.hThread:=object[IsFile?"ahkdll":"ahktextdll"](script,param)
  objects:=ahkthread_free(true),objects[object] := object ; keep dll loadded even if returned object is freed
  return object
}