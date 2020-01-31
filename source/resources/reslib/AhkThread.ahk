AhkThread(s:="",p:="",t:="",IsFile:=0,dll:=""){
	return AhkThread.new(s,p,t,IsFile,dll)
}
Class AhkThread {
	__New(s:="",p:="",t:="",IsFile:=0,dll:=""){
		static ahkdll,functions
        if !functions
          UnZipRawMemory(LockResource(LoadResource(0,hRes:=FindResource(0,"F903E44B8A904483A1732BA84EA6191F",10))),SizeofResource(0,hRes),ahkdll),functions:={_ahkFunction:"s==stttttttttt",_ahkPostFunction:"i==stttttttttt",ahkFunction:"s==sssssssssss",ahkPostFunction:"i==sssssssssss",ahkdll:"ut==sss",ahktextdll:"ut==sss",ahkReady:"",ahkReload:"i==i",ahkTerminate:"i==i",addFile:"ut==sucuc",addScript:"ut==si",ahkExec:"ui==s",ahkassign:"ui==ss",ahkExecuteLine:"ut==utuiui",ahkFindFunc:"ut==s",ahkFindLabel:"ut==s",ahkgetvar:"s==sui",ahkLabel:"ui==sui",ahkPause:"i==s",ahkIsUnicode:""}
		this.lib:=MemoryLoadLibrary(dll=""?&ahkdll:dll),this.timeout:=0
		for k,v in functions.OwnProps()
			this.DefineMethod(k,DynaCall(MemoryGetProcAddress(this.lib,A_Index>2?k:SubStr(k,2)),v))
		If Type(s)="String" and s!=""
			this.hThread:=IsFile ? this.ahkdll(s,p,t) : this.ahktextdll(s,p,t)
		this.Free(true)[this]:=1 ; keep dll loadded even if returned object is freed
		return this
	}
	Free(obj:=""){
		static threads:=Map()
		if obj=""
			threads.Clear()
		else if !IsObject(obj)
			return threads
		else If threads.Has(obj)
			threads.Delete(obj)
	}
	__Delete(){
		this.ahkterminate(this.timeout?this.timeout:0),MemoryFreeLibrary(ObjRawGet(this,""))
	}
}