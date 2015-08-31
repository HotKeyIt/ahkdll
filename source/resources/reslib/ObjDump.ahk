ObjDump(obj,ByRef var:="",mode:=0){
  If IsObject(var){ ; FileAppend mode
    If FileExist(obj)&&!FileDelete(obj),return
    f:=FileOpen(obj,"rw-rwd"),VarSetCapacity(v,sz:=RawObjectSize(var,mode)+8,0)
    ,RawObject(var,NumPut(sz-8,ptr:=&v,"Int64"),mode),count:=sz//65536
    Loop count
      f.RawWrite(ptr+0,65536),ptr+=65536
    return f.RawWrite(ptr+0,Mod(sz,65536)),f.Close(),sz
  } else if !IsByRef(var)       ,return RawObjectSize(obj,mode)+8
  else return VarSetCapacity(var,sz:=RawObjectSize(obj,mode)+8,0),RawObject(obj,NumPut(sz-8,&var,"Int64"),mode),sz
}
RawObject(obj,addr,buf:=0){
  ; Type.Enum:    Char.1 UChar.2 Short.3 UShort.4 Int.5 UInt.6 Int64.7 UInt64.8 Double.9 String.10 Object.11
  ; Negative for keys and positive for values
  for k,v in obj
  { ; 9 = Int64 for size and Char for type
    If IsObject(k),NumPut(-11,addr,"Char"),NumPut(sz:=RawObjectSize(k,buf),addr+1,"Int64"),RawObject(k,addr+9,buf),addr+=sz+9
    else if Type(k)="String",NumPut(-10,addr,"Char"),NumPut(sz:=StrPut(k,addr+9)*2,addr+1,"Int64"),addr+=sz+9
    else NumPut( InStr(k,".")?-9:k>4294967295?-8:k>65535?-6:k>255?-4:k>-1?-2:k>-129?-1:k>-32769?-3:k>-2147483649?-5:-7,addr,"Char")
        ,NumPut(k,addr+1,InStr(k,".")?"Double":k>4294967295?"UInt64":k>65535?"UInt":k>255?"UShort":k>-1?"UChar":k>-129?"Char":k>-32769?"Short":k>-2147483649?"Int":"Int64")
        ,addr+=InStr(k,".")||k>4294967295?9:k>65535?5:k>255?3:k>-129?2:k>-32769?3:k>-2147483649?5:9
    If IsObject(v),NumPut( 11,addr,"Char"),NumPut(sz:=RawObjectSize(v,buf),addr+1,"Int64"),RawObject(v,addr+9,buf),addr+=sz+9
    else if Type(v)="String",NumPut( 10,addr,"Char"),NumPut(sz:=buf?obj.GetCapacity(k):StrPut(v)*2,addr+1,"Int64"),DllCall("RtlMoveMemory","PTR",addr+9,"PTR",&v,"PTR",sz),addr+=sz+9
    else NumPut(InStr(v,".")?9:v>4294967295?8:v>65535?6:v>255?4:v>-1?2:v>-129?1:v>-32769?3:v>-2147483649?5:7,addr,"Char")
        ,NumPut(v,addr+1,InStr(v,".")?"Double":v>4294967295?"UInt64":v>65535?"UInt":v>255?"UShort":v>-1?"UChar":v>-129?"Char":v>-32769?"Short":v>-2147483649?"Int":"Int64")
        ,addr+=InStr(v,".")||v>4294967295?9:v>65535?5:v>255?3:v>-129?2:v>-32769?3:v>-2147483649?5:9
  }
}
RawObjectSize(obj,buf:=0,sz:=0){
  for k,v in obj
  {
    If IsObject(k),sz+=RawObjectSize(k,buf)+9
      else if Type(k)="String",sz+=StrPut(k)*2+9
      else sz+=InStr(k,".")||k>4294967295?9:k>65535?5:k>255?3:k>-129?2:k>-32769?3:k>-2147483649?5:9
    If IsObject(v),sz+=RawObjectSize(v,buf)+9
      else if Type(v)="String",sz+=(buf?obj.GetCapacity(k):StrPut(v)*2)+9
      else sz+=InStr(v,".")||v>4294967295?9:v>65535?5:v>255?3:v>-129?2:v>-32769?3:v>-2147483649?5:9
  }
  return sz
}