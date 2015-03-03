A_NowUTC(b:=0,n:=0){
static t:=Struct("WORD Y;WORD M;WORD WD;WORD D;WORD H;WORD N;WORD S;WORD MS"),f:="{1:04d}{2:02d}{3:02d}{4:02d}{5:02d}{6:02d}",g:=DynaCall("GetSystemTime","t")
Critical
G[t[]]
return b?StrPut(format(f,t.Y,t.M,t.D,t.H,t.N,t.S),b):n?14:format(f,t.Y,t.M,t.D,t.H,t.N,t.S)
}