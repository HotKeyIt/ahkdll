ahkthread_free(obj:=""){
  static objects
  if !objects
    objects:=[]
  if obj=""
    objects:=[]
  else if !IsObject(obj)
    return objects
  else If objects.HasKey(obj)
    objects.Remove(obj)
}
ahkthread_release(o){
  o.ahkterminate(o.timeout?o.timeout:0),MemoryFreeLibrary(o[""])
}
ahkthread(s:="",p:="",t:="",IsFile:=0,dll:=""){
  static ahkdll,base,functions
  if !base
    base:={__Delete:"ahkthread_release"},UnZipRawMemory(LockResource(LoadResource(0,hRes:=FindResource(0,"F903E44B8A904483A1732BA84EA6191F",10))),SizeofResource(0,hRes),ahkdll)
    ,functions:={_ahkFunction:"s==stttttttttt",_ahkPostFunction:"i==stttttttttt",ahkFunction:"s==sssssssssss",ahkPostFunction:"i==sssssssssss",ahkdll:"ut==sss",ahktextdll:"ut==sss",ahkReady:"",ahkReload:"i==i",ahkTerminate:"i==i",addFile:"ut==sucuc",addScript:"ut==si",ahkExec:"ui==s",ahkassign:"ui==ss",ahkExecuteLine:"ut==utuiui",ahkFindFunc:"ut==s",ahkFindLabel:"ut==s",ahkgetvar:"s==sui",ahkLabel:"ui==sui",ahkPause:"i==s",ahkIsUnicode:""}
  obj:={(""):lib:=MemoryLoadLibrary(dll=""?&ahkdll:dll),base:base}
  for k,v in functions
    obj[k]:=DynaCall(MemoryGetProcAddress(lib,A_Index>2?k:SubStr(k,2)),v)
  If !(s+0!="" || s=0)
    obj.hThread:=obj[IsFile?"ahkdll":"ahktextdll"](s,p,t)
  ahkthread_free(true)[obj]:=1 ; keep dll loadded even if returned object is freed
  return obj
}