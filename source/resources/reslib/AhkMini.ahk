ahkmini(script:="",param:="",IsFile:=0,dll:="FC2328B39C194A4788051A3B01B1E7D5"){
static ahkdll
if !ahkdll
hRes:=DllCall("FindResource","PTR",0,"STR","FC2328B39C194A4788051A3B01B1E7D5","PTR",10,"PTR"),data:=LockResource(hResData:=LoadResource(0,hRes)),UnZipRawMemory(data,SizeofResource(0,hRes),ahkdll)
return ahkthread(script,param,IsFile,dll+0?dll:dll="FC2328B39C194A4788051A3B01B1E7D5"?&ahkdll:dll)
}