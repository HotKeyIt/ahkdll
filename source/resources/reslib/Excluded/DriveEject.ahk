DriveEject(d){
if !d{
ErrorLevel:=DllCall("winmm\mciSendString","Str","set cdaudio door waitopen","PTR",0,"UInt",0,"PTR",0)
return
}
if DllCall("winmm\mciSendString","Str","open " d " type cdaudio alias cd wait shareable","PTR",0,"UInt",0,"PTR",0){
ErrorLevel:=1
return
}
r:=DllCall("winmm\mciSendString","Str","set cd door " d " waitopen","PTR",0,"UInt",0,"PTR",0)
DllCall("winmm\mciSendString","Str","close cd wait","PTR",0,"UInt",0,"PTR",0)
Errorlevel:=r
}