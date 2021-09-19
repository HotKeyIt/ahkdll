MCodeH(h,def,p*){
static f:=[],DynaCalls:=Map()
If DynaCalls.Has(h)
	return DynaCalls[h]
f.Push(buf:=BufferAlloc(len:=StrLen(h)//2))
DllCall("VirtualProtect","PTR",addr:=buf.ptr,"Uint",len,"UInt",64,"Uint*",0)
Loop len
	NumPut("Char","0x" SubStr(h,2*A_Index-1,2),addr,A_Index-1)
if p.Length
	Return DynaCalls[h]:=DynaCall(addr,def,p*)
else Return DynaCalls[h]:=DynaCall(addr,def)
}