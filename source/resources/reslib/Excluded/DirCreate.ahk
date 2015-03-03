DirCreate(szDirName,ByRef error:=0){ ; Recursive directory creation function.
	dwTemp := DllCall("GetFileAttributes","Str",szDirName,"UInt")
	if (dwTemp = 0xffffffff && lastError:=A_LastError){	; error getting attribute - what was the error?
		If lastError = 3{ ;ERROR_PATH_NOT_FOUND
			; Create path
			length := StrLen(szDirName)
			if (length > 260){ ; MAX_PATH Sanity check to reduce chance of stack overflow (since this function recursively calls self).
        error:=ErrorLevel:=2
				return
      }
			If !psz_Loc := InStr(szDirName,"\",1,-1){
        error:=ErrorLevel:=1
        return
      }
			szTemp := szDirName
      NumPut(0,szTemp,psz_Loc*2-2,"Short")				; remove \ and everything after
      if !DirCreate(szTemp)&&error{
        error:=ErrorLevel:=1
        return
      }
      error:=ErrorLevel:=DllCall("CreateDirectory","Str",szDirName,"PTR",0)?0:1
			return
			; All paths above "return".
		} else if lastError = 2 { ;ERROR_FILE_NOT_FOUND
			; Create directory
      error:=ErrorLevel:=DllCall("CreateDirectory","Str",szDirName,"PTR", 0)?0:1
			return
		; Otherwise, it's some unforeseen error, so fall through to the end, which reports failure.
    }
	}	else ; The specified name already exists as a file or directory.
		if (dwTemp & 16){ ; Fixed for v1.0.36.01 (previously it used == vs &). //FILE_ATTRIBUTE_DIRECTORY:=16
      error:=ErrorLevel:=0
			return							; Directory exists, yay!
    }
		;else it exists, but it's a file! Not allowed, so fall through and report failure.
	error:=ErrorLevel := 1		
}