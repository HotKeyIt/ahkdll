A_IPAddress2(b:=0,n:=0){
static w:=Struct("WORD wVersion,WORD wHighVersion,char szDescription[257],char szSystemStatus[129],USHORT iMaxSockets,USHORT iMaxUdpDg,char *lpVendorInfo")
,temp:=VarSetCapacity(start,2),host_name:=Struct("char[256]")
,i:=Struct("{struct{byte b1,byte b2,byte b3,byte b4},{ushort s_w1,ushort s_w2},ulong S_addr}")
,l:=Struct("byte *h_name,byte **h_aliases,short h_addrtype,short h_length,UINT *addr")
,WSA:=DynaCall("Ws2_32\WSAStartup","ht",1,w[]),GHN:=DynaCall("Ws2_32\gethostname","tui",host_name[],256)
,GHBN:=DynaCall("Ws2_32\gethostbyname","t=t",hostent[]),WSAC:=DynaCall("Ws2_32\WSACleanup")
Critical
if (n&&!b)||WSA[]
return 16
GHN[],l[]:=GHBN[],c:=0
while (l.addr[c+1])
++c
if (2 > c)
return WSAC[],!n?"0.0.0.0":StrPut("0.0.0.0",b)
i[]:=l.addr.1,ip:=i.b1 "." i.b2 "." i.b3 "." i.b4
if n
return WSAC[],StrPut(ip,b)
else return WSAC[],ip
}