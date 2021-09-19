CriticalSection(cs:=0){
  static i:=0,crisec:={base:{__Delete:CriticalSection}}
  if IsObject(cs){
    Loop i
      DeleteCriticalSection(cs.%i%.Ptr),cs.DeleteProp("" i)
    Return i:=0
  } else if cs
    return (DeleteCriticalSection(cs),cs)
  Return (i++,crisec.%i%:=Buffer(A_PtrSize=8?40:24),InitializeCriticalSection(crisec.%i%.Ptr),crisec.%i%.Ptr)
}
