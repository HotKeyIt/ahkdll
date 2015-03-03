criticalsection(cs:=0){
static 
static count:=0,base:={base:{__Delete:"criticalsection"}}
if cs==base.base{
Loop count
DllCall("DeleteCriticalSection",PTR,&CriticalSection%count%),count:=0
Return
} else if cs
return DllCall("DeleteCriticalSection",PTR,cs)
count++,VarSetCapacity(CriticalSection%count%,24),DllCall("InitializeCriticalSection",PTR,&CriticalSection%count%)
Return &CriticalSection%count%
}