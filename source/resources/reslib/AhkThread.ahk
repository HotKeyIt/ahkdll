ahkthread_free(obj:=""){
	static objects:=[]
	if obj="",objects:=[]
	else if !IsObject(obj)
		return objects
	else If objects.HasKey(obj)
		objects.Remove(obj)
}
ahkthread_release(o){
  o.ahkterminate(o.timeout?o.timeout:0),MemoryFreeLibrary(o[""])
}
ahkthread(s:="",p:="",IsFile:=0,dll:="F903E44B8A904483A1732BA84EA6191F"){
  static ahkdll,base
  if !base
    base:={__Delete:"ahkthread_release"},UnZipRawMemory(LockResource(LoadResource(0,hRes:=FindResource(0,"F903E44B8A904483A1732BA84EA6191F",10))),SizeofResource(0,hRes),ahkdll)
  obj:={(""):lib:=MemoryLoadLibrary(dll="F903E44B8A904483A1732BA84EA6191F"?&ahkdll:dll),base:base}
  for k,v in {_ahkFunction:"s==stttttttttt",_ahkPostFunction:"i==stttttttttt",ahkFunction:"s==sssssssssss",ahkPostFunction:"i==sssssssssss",ahkdll:"ut==ss",ahktextdll:"ut==ss",ahkReady:"",ahkReload:"i==i",ahkTerminate:"i==i",addFile:"ut==sucuc",addScript:"ut==si",ahkExec:"ui==s",ahkassign:"ui==ss",ahkExecuteLine:"ut==utuiui",ahkFindFunc:"ut==s",ahkFindLabel:"ut==s",ahkgetvar:"s==sui",ahkLabel:"ui==sui",ahkPause:"i==s",ahkIsUnicode:""}
    obj[k]:=DynaCall(MemoryGetProcAddress(lib,A_Index>2?k:SubStr(k,2)),v)
  If !(s+0!="" || s=0)
    obj.hThread:=obj[IsFile?"ahkdll":"ahktextdll"](s,p)
  ahkthread_free(true)[obj]:=1 ; keep dll loadded even if returned object is freed
  return obj
}