DriveGetStatusCD(d){
static s,i:=VarSetCapacity(s,256)
if !d=""
return DllCall("winmm\mciSendString","Str","status cdaudio mode","PTR",&s,"Int",128,"PTR",0)?0:1
else {
if DllCall("winmm\mciSendString","Str","open " d " type cdaudio alias cd wait shareable","PTR",0,"Int",0,"PTR",0)
return 0
e:=DllCall("winmm\mciSendString","Str","status cd mode","PTR",&s,"Int",128,"PTR",0)
DllCall("winmm\mciSendString","Str","close cd wait","PTR",0,"Int",0,"PTR",0)
if (e)
return -1
else VarSetCapacity(s,-1)
return s
}
}
