SoundBeep(aFreq:=0,aDur:=0){
DllCall("Beep","UInt",aFreq?aFreq:523,"UInt"aDur?aDur:150)
}