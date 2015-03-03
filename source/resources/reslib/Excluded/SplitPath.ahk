SplitPath(aFileSpec,ByRef output_var_name:="",ByRef output_var_dir:="",ByRef output_var_ext:="",ByRef output_var_name_no_ext:="",ByRef output_var_drive:=""){
	; For URLs, "drive" is defined as the server name, e.g. http:;somedomain.com
	;~ LPTSTR name = _T(""), name_delimiter = NULL, drive_end = NULL ; Set defaults to improve maintainability.
	drive := ltrim(aFileSpec) ; i.e. whitespace is considered for everything except the drive letter or server name, so that a pathless filename can have leading whitespace.
	
	colon_double_slash := InStr(aFileSpec, ":/") ? SubStr(aFileSpec,InStr(aFileSpec, ":/")+3) : ""
	
	if (colon_double_slash) ; This is a URL such as ftp:;... or http:;...
	{
		if !(drive_end := InStr(colon_double_slash, "/"))
		{
			if !(drive_end := InStr(colon_double_slash,"\\")) ; Try backslash so that things like file:;C:\Folder\File.txt are supported.
				drive_end := StrLen(colon_double_slash) ; Set it to the position of the zero terminator instead.
				; And because there is no filename, leave name and name_delimiter set to their defaults.
			;else there is a backslash, e.g. file:;C:\Folder\File.txt, so treat that backslash as the end of the drive name.
		}
		name_delimiter := SubStr(colon_double_slash,drive_end,1) ; Set default, to be possibly overridden below.
		; Above has set drive_end to one of the following:
		; 1) The slash that occurs to the right of the doubleslash in a URL.
		; 2) The backslash that occurs to the right of the doubleslash in a URL.
		; 3) The zero terminator if there is no slash or backslash to the right of the doubleslash.
		if (drive_end) ; A slash or backslash exists to the right of the server name.
		{
			if SubStr(aFileSpec,drive_end+1)
			{
				; Find the rightmost slash.  At this stage, this is known to find the correct slash.
				; In the case of a file at the root of a domain such as http:;domain.com/root_file.htm,
				; the directory consists of only the domain name, e.g. http:;domain.com.  This is because
				; the directory always the "drive letter" by design, since that is more often what the
				; caller wants.  A script can use StringReplace to remove the drive/server portion from
				; the directory, if desired.
				name_delimiter := SubStr(aFileSpec,InStr(aFileSpec, "/"),1)
				if (name_delimiter = SubStr(colon_double_slash,3,1)) ; To reach this point, it must have a backslash, something like file:;c:\folder\file.txt
					name_delimiter := SubStr(aFileSpec, InStr(aFileSpec,"\")) ; Will always be found.
				name := SubStr(name_delimiter,2) ; This will be the empty string for something like http:;domain.com/dir/
			}
			;else something like http:;domain.com/, so leave name and name_delimiter set to their defaults.
		}
		;else something like http:;domain.com, so leave name and name_delimiter set to their defaults.
	}
	else ; It's not a URL, just a file specification such as c:\my folder\my file.txt, or \\server01\folder\file.txt
	{
		; Differences between _splitpath() and the method used here:
		; _splitpath() doesn't include drive in output_var_dir, it includes a trailing
		; backslash, it includes the . in the extension, it considers ":" to be a filename.
		; _splitpath(pathname, drive, dir, file, ext)
		;char sdrive[16], sdir[MAX_PATH], sname[MAX_PATH], sext[MAX_PATH]
		;_splitpath(aFileSpec, sdrive, sdir, sname, sext)
		;if (output_var_name_no_ext)
		;	output_var_name_no_ext->Assign(sname)
		;strcat(sname, sext)
		;if (output_var_name)
		;	output_var_name->Assign(sname)
		;if (output_var_dir)
		;	output_var_dir->Assign(sdir)
		;if (output_var_ext)
		;	output_var_ext->Assign(sext)
		;if (output_var_drive)
		;	output_var_drive->Assign(sdrive)
		;return OK

		; Don't use _splitpath() since it supposedly doesn't handle UNC paths correctly,
		; and anyway we need more info than it provides.  Also note that it is possible
		; for a file to begin with space(s) or a dot (if created programmatically), so
		; don't trim or omit leading space unless it's known to be an absolute path.

		; Note that "C:Some File.txt" is a valid filename in some contexts, which the below
		; tries to take into account.  However, there will be no way for this command to
		; return a path that differentiates between "C:Some File.txt" and "C:\Some File.txt"
		; since the first backslash is not included with the returned path, even if it's
		; the root directory (i.e. "C:" is returned in both cases).  The "C:Filename"
		; convention is pretty rare, and anyway this trait can be detected via something like
		; IfInString, Filespec, :, IfNotInString, Filespec, :\, MsgBox Drive with no absolute path.

		; UNCs are detected with this approach so that double sets of backslashes -- which sometimes
		; occur by accident in "built filespecs" and are tolerated by the OS -- are not falsely
		; detected as UNCs.
		if (SubStr(drive,1,2) = "\\") ; Relies on short-circuit evaluation order.
		{
			if !(drive_end := InStr(drive, "\",1,2))
				drive_end := StrLen(drive) ; Set it to the position of the zero terminator instead.
		}
		else if (SubStr(drive,2,1) = ":") ; It's an absolute path.
			; Assign letter and colon for consistency with server naming convention above.
			; i.e. so that server name and drive can be used without having to worry about
			; whether it needs a colon added or not.
			drive_end := 2
		else
		{
			; It's debatable, but it seems best to return a blank drive if a aFileSpec is a relative path.
			; rather than trying to use GetFullPathName() on a potentially non-existent file/dir.
			; _splitpath() doesn't fetch the drive letter of relative paths either.  This also reports
			; a blank drive for something like file:;C:\My Folder\My File.txt, which seems too rarely
			; to justify a special mode.
			drive_end := 0
			drive := "" ; This is necessary to allow Assign() to work correctly later below, since it interprets a length of zero as "use string's entire length".
		}

		if !(name_delimiter := InStr(aFileSpec, "\",1,-1)) ; No backslash.
			if !(name_delimiter := InStr(aFileSpec, ":")) ; No colon.
				name_delimiter := 0 ; Indicate that there is no directory.

		name := name_delimiter ? SubStr(aFileSpec,name_delimiter+1) : aFileSpec ; If no delimiter, name is the entire string.
	}

	; The above has now set the following variables:
	; name: As an empty string or the actual name of the file, including extension.
	; name_delimiter: As NULL if there is no directory, otherwise, the end of the directory's name.
	; drive: As the start of the drive/server name, e.g. C:, \\Workstation01, http:;domain.com, etc.
	; drive_end: As the position after the drive's last character, either a zero terminator, slash, or backslash.

	if IsByRef(output_var_name)
		output_var_name:=name
	if IsByRef(output_var_dir){
		if (!name_delimiter)
			output_var_dir:="" ; Shouldn't fail.
		else if (SubStr(aFileSpec,name_delimiter,1) = "\" || SubStr(aFileSpec,name_delimiter,1) = "/")
			output_var_dir:=SubStr(aFileSpec,1,name_delimiter - 1)
		else ; *name_delimiter == ':', e.g. "C:Some File.txt".  If aFileSpec starts with just ":",
			 ; the dir returned here will also start with just ":" since that's rare & illegal anyway.
			output_var_dir:=SubStr(aFileSpec,1,name_delimiter)
	}

	ext_dot := InStr(name, ".",1,-1)
	if IsByRef(output_var_ext){
		; Note that the OS doesn't allow filenames to end in a period.
		if (!ext_dot)
			output_var_ext:=""
		else
			output_var_ext:=SubStr(name,ext_dot + 1) ; Can be empty string if filename ends in just a dot.
	}

	if IsByRef(output_var_name_no_ext)
		output_var_name_no_ext:=SubStr(name, 1,ext_dot ? ext_dot-1 : StrLen(name))

	if IsByRef(output_var_drive)
		output_var_drive:=SubStr(drive,1, drive_end )
}
