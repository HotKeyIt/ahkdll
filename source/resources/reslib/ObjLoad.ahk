ObjLoad(addr){
  If (addr+0=""){ ; FileRead Mode
    If !FileExist(addr)
      return
    FileGetSize,sz,%addr%
    FileRead,v,*c %addr%
    If ErrorLevel||!sz
      return
    addr:=&v
  }
  obj:=[],end:=addr+8+(sz:=NumGet(addr+0,"Int64")),addr+=8
  While addr<end{ ; 9 = Int64 for size and Char for type
    If NumGet(addr+0,"Char")=-11
			k:=ObjLoad(++addr),addr+=8+NumGet(addr+0,"Int64")
    else if NumGet(addr+0,"Char")=-10
			sz:=NumGet(addr+1,"Int64"),k:=StrGet(addr+9),addr+=sz+9
    else k:=NumGet(addr+1,SubStr("Char  UChar Short UShortInt   UInt  Int64 UInt64Double",(sz:=-NumGet(addr+0,"Char"))*6-5,6)),addr+=SubStr("112244888",sz,1)+1
    If NumGet(addr+0,"Char")= 11
			obj[k]:=ObjLoad(++addr),addr+=8+NumGet(addr+0,"Int64")
    else if NumGet(addr+0,"Char")= 10
			obj.SetCapacity(k,sz:=NumGet(addr+1,"Int64")),obj[k]:=StrGet(addr+9),DllCall("RtlMoveMemory","PTR",obj.GetAddress(k),"PTR",addr+9,"PTR",sz),addr+=sz+9
    else obj[k]:=NumGet(addr+1,SubStr("Char  UChar Short UShortInt   UInt  Int64 UInt64Double",(sz:=NumGet(addr+0,"Char"))*6-5,6)),addr+=SubStr("112244888",sz,1)+1
  }
  return obj
}