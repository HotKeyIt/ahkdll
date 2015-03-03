SoundPlay(aFilespec,aSleepUntilDone:=0){
	static MAX_PATH:=260,SOUNDPLAY_ALIAS:="AHK_PlayMe",init1:=VarSetCapacity(buf,MAX_PATH * 2) ; Allow room for filename and commands.
				,ClosePlayOnExit
	if (SubStr(trim(aFilespec),1,1)="*"){
		ErrorLevel:=!DllCall("MessageBeep","Int",SubStr(aFilespec,2))
		return
	}
	ErrorLevel:=1
		; ATOU() returns 0xFFFFFFFF for -1, which is relied upon to support the -1 sound.
	; See http:;msdn.microsoft.com/library/default.asp?url=/library/en-us/multimed/htm/_win32_play.asp
	; for some documentation mciSendString() and related.
	DllCall("Winmm\mciSendString","Str","status " SOUNDPLAY_ALIAS " mode","PTR", *buf,"Int", MAX_PATH * 2, "Ptr", 0)
	If StrGet(&buf)
	;~ if (*buf) ; "playing" or "stopped" (so close it before trying to re-open with a new aFilespec).
		DllCall("Winmm\mciSendString","Str","close " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0)
	if DllCall("Winmm\mciSendString","Str","open `"" aFilespec "`" alias " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0) ; Failure.
		return
	;~ g_SoundWasPlayed = true  ; For use by Script's destructor.
	if DllCall("Winmm\mciSendString","Str","play " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0){ ; Failure.
		DllCall("Winmm\mciSendString","Str","close " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0)
		return
	}
	; Otherwise, the sound is now playing.
	;~ g_ErrorLevel->Assign(ERRORLEVEL_NONE)
	ErrorLevel:=0
	if (!aSleepUntilDone)
	{
		ClosePlayOnExit:=new SoundPlayCloseOnExit
		return
	} else ClosePlayOnExit:=""
	; Otherwise, caller wants us to wait until the file is done playing.  To allow our app to remain
	; responsive during this time, use a loop that checks our message queue:
	; Older method: "mciSendString("play " SOUNDPLAY_ALIAS " wait", NULL, 0, NULL)"
	Loop {
		DllCall("Winmm\mciSendString","Str","status " SOUNDPLAY_ALIAS " mode","PTR", &buf,"Int", MAX_PATH * 2,"PTR", 0)
		if !StrGet(&buf) ; Probably can't happen given the state we're in.
			break
		else if (StrGet(&buf) = "stopped"){ ; The sound is done playing.
			DllCall("Winmm\mciSendString","Str","close " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0)
			break
		}
		; Sleep a little longer than normal because I'm not sure how much overhead
		; and CPU utilization the above incurs:
		Sleep, 20
	}
	return
}
Class SoundPlayCloseOnExit {
	__Delete(){
		static SOUNDPLAY_ALIAS:="AHK_PlayMe",init1:=VarSetCapacity(buf,MAX_PATH)
		DllCall("Winmm\mciSendString","Str","status " SOUNDPLAY_ALIAS " mode","PTR", &buf,"Int", MAX_PATH,"PTR", 0)
		if !StrGet(&buf) ; Probably can't happen given the state we're in.
			return
		else if (StrGet(&buf) = "stopped"){ ; The sound is done playing.
			DllCall("Winmm\mciSendString","Str","close " SOUNDPLAY_ALIAS,"PTR", 0,"Int", 0,"PTR", 0)
			return
		}
	}
}
