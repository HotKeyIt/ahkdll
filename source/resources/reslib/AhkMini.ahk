ahkmini(script:="",param:="",IsFile:=0,dll:="FC2328B39C194A4788051A3B01B1E7D5"){
static ahkdll,hRes:=FindResourceW(0,"FC2328B39C194A4788051A3B01B1E7D5",10),data:=LockResource(LoadResource(0,hRes)),init:=UnZipRawMemory(data,SizeofResource(0,hRes),ahkdll)
return ahkthread(script,param,IsFile,dll+0?dll:dll="FC2328B39C194A4788051A3B01B1E7D5"?&ahkdll:dll)
}