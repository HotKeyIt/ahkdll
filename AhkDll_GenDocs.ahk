;: Title: AhkDll by tinku99 powered by HotKeyIt
;

; Function: AutoHotkey.dll
; Description:
;      AutoHotkey.dll was invented by <a href=http://www.autohotkey.com/forum/profile.php?mode=viewprofile&u=6056>tinku99</a> and enhanced by <a href=http://www.autohotkey.com/forum/profile.php?mode=viewprofile&u=9238>HotKeyIt</a>, it is a custom build of <a href=http://www.autohotkey.com/forum/viewtopic.php?p=210161#210161>AutoHotkey_L</a> (by <a href=http://www.autohotkey.com/forum/profile.php?mode=viewprofile&u=3754>Lexikos</a>).<br>AutoHotkey.dll can be used in other programming languages and allows multithreading in AutoHotkey.<br><b><a href=http://www.autohotkey.net/~HotKeyIt/AutoHotkey.zip>GET AutoHotkey V1 PACKAGE HERE.</a><br><a href=http://www.autohotkey.net/~HotKeyIt/AutoHotkey2Alpha.zip>GET AutoHotkey V2alpha PACKAGE HERE.</a></b>
; Syntax: DllCall(dll_path "\" "function","UInt/Str/...",...,"Cdecl ...")
; Parameters:
;	   Get started - To use AutoHotkey.dll you need AutoHotkey.exe or anther program able to load and run a dll or use COM.<br>In AutoHotkey.exe, AutoHotkey.dll is loaded using <b>DllCall("LoadLibrary","Str","dllpath or name")</b>, you can also use AutoHotkey.dll COM Interface.<br>Other languages and programs that support loading dlls have similar functions. (<a href=http://www.autohotkey.com/forum/viewtopic.php?t=61457>python</a>, <a href=http://www.autohotkey.com/forum/viewtopic.php?p=339622#339622>c#/c++</a>)<br><br>Each loaded AutoHotkey.dll will serve an additional thread with full AutoHotkey functionality.<br><br>To unload AutoHotkey.dll you can use DllCall("FreeLibrary","UInt",handleLibrary).<br>To start the actual thrad you will need to use ahkdll or ahktextdll (exported functions). Click the link below to read more about ahkdll and ahktextdll.
;	   Why AutoHotkey.dll - <b>Multithreading</b><br>We can run multiple threads simultaneously by loading several dlls. For example you can use Input command to catch user input and perform your task in background so no Input gets lost and your code works uninterupted.<br>It is even possible to use same variables and objects using Alias() function, but be careful, when variable gets reallocated because its memory increases and you are accessing it, your program might crash, therefore you can use <a href=http://www.autohotkey.com/forum/topic51335.html>CriticalSection()</a> to avoid simultaneous access.<br><br>
;	   AutoHotkey.dll <b>Exported Functions</b> - ahkdll = start new thread from ahk file. <b>(not available in AutoHotkey.exe)</b><br>ahktextdll = start new thread from string that contains an ahk script. <b>(not available in AutoHotkey.exe)</b><br>ahkReady = check if a script is running.<br>addFile = add new script(lines) to existing(running) script from a file.<br>addScript = add new script(lines) from string that contains an ahk script.<br>ahkExec = execute a script from a string that contains ahk script.<br>ahkLabel = jump(GoTo/GoSub) to a label.<br>ahkFunction = Call a function and return result.<br>ahkPostFunction = call function without getting result and continuing with script immediately.<br>ahkassign = assign new value to a variable<br>ahkgetvar = get value of a variable<br>ahkTerminate = terminate the script/thread <b>(not available in AutoHotkey.exe)</b><br>ahkReload = reload script<br>ahkFindFunc = find a function and return a handle to it.<br>ahkFindLabel = find a label and return a handle to it.<br>ahkPause = pause the script.<br>ahkExecuteLine = Execute script from certain line or get first/next line.<br><br>
;	   AutoHotkey.dll <b>COM interface</b> - AutoHotkey.dll includes a COM Interface so you can use AutoHotkey trough COM, see examples below.<br>Following interfaces are available:<br>AutoHotkey V1:<br> - AutoHotkey.Script = last registered (Unicode/Ansi/X64)<br> - AutoHotkey.Script.UNICODE = unicode win32<br> - AutoHotkey.Script.ANSI = ansi win32<br> - AutoHotkey.Script.X64 = unicode x64<br>AutoHotkey V2:<br> - AutoHotkey2.Script = last registered (Unicode/X64)<br> - AutoHotkey2.Script.UNICODE = unicode win32<br> - AutoHotkey2.Script.X64 = unicode x64
; Return Value:
;     See function help.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_General.ahk
;

;
; Function: ahkdll
; Description:
;      ahkdll is used to launch a file containing AutoHotkey script in a separate thread using AutoHotkey.dll. ( Available in AutoHotkey[Mini].dll only, not in AutoHotkey_H.exe)
; Syntax: hThread:=DllCall(dll_path "\<b>ahkdll</b>","Str","MyScript.ahk","Str",options,"Str",parameters,"Cdecl UPTR")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   hThread - ahkdll() returns a thread handle.
;	   "MyScript.ahk" - This parameter must be an existing ahk file.
;	   options - Within the script you can access this parameter via build in variable A_ScriptOptions.
;	   (parameters) - Parameters passed to dll. Similar to <b>Run myscript.exe "parameters"<b>
; Return Value:
;     <b>Cdecl UPTR</b> - ahkdll returns a thread handle, using this handle you can use functions like DllCall("SuspendThread/ResumeThread/GetExitCode",...)... for your thread.
; Remarks:
;		Don't forget to load AutoHotkey.dll via DllCall("LoadLibrary"...), otherwise it will be automatically Freed by AutoHotkey.exe after DllCall finished.
; Related:
; Example:
;		file:Example_ahkdll.ahk
;

;
; Function: ahktextdll
; Description:
;      ahktextdll is used to launch a script in a separate thread using AutoHotkey.dll from text/variable. ( Available in AutoHotkey[Mini].dll only, not in AutoHotkey_H.exe)
; Syntax: hThread:=DllCall(dll_path "\<b>ahktextdll</b>","Str","MsgBox % dll_path`nMsgBox % A_Now`nReturn","Str",options,"Str",parameters,"Cdecl UPTR")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   hThread - ahktextdll() returns a thread handle.
;	   ("MsgBox...") - This parameter must be an AutoHotkey script as text or variable containing a script as text.
;	   (options) - Within the script you can access this parameter via build in variable A_ScriptOptions.
;	   (parameters) - Parameters passed to dll. Similar to <b>Run myscript.exe "parameters"<b>
; Return Value:
;     <b>Cdecl UPTR</b> - ahkdll returns a thread handle, using this handle you can use functions like DllCall("SuspendThread/ResumeThread/GetExitCode",...)... for your thread.
; Remarks:
;		Don't forget to load AutoHotkey.dll via DllCall("LoadLibrary"...), otherwise it will be automatically Freed by AutoHotkey.exe after DllCall finished.
; Related:
; Example:
;		file:Example_ahktextdll.ahk
;

;
; Function: ahkReady
; Description:
;      ahkReady is used to check if a dll script is running or not. ( Available in AutoHotkey[Mini].dll only, not in AutoHotkey_H.exe)
; Syntax: ThreadIsRunning:=DllCall(dll_path "\<b>ahkReady</b>")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   ThreadIsRunning - ahkReady() returns 1 if a thread is running or 0 otherwise.
; Return Value:
;     1 or 0.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_ahkReady.ahk
;

;
; Function: addFile
; Description:
;      addFile is used to append additional labels and functions from a file to the running script.
; Syntax: pointerLine:=DllCall(dll_path "\<b>addFile</b>","Str",file[,"Uchar",AllowDuplicateInclude,"Uchar",IgnoreLoadFailure],"Cdecl UPTR")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   pointerLine - addFile() returns a pointer to the first line in the late included script.
;	   file - name of script to be parsed and added to the running script
;	   AllowDuplicateInclude - 0 = if a script named %filename% is already loaded, ignore (do not load)<br>1 = load it again
;	   IgnoreLoadFailure - 0 = signal an error if there was problem addfileing filename<br>1 = ignore error<br>2 = remove script lines added by previous calls to addFile() and start executing at the first line in the new script [AutoHotkey.dll only]
; Return Value:
;     <b>Cdecl UInt</b> - return value is a pointer to the first line of new created lines.
; Remarks:
;		Line pointer can be used in ahkExecuteLine() to execute one line only or until a return is encountered.
; Related:
; Example:
;		file:Example_addFile.ahk
;

;
; Function: addScript
; Description:
;      addScript is used to append additional labels and functions from text or variable to the running script from text/variable.
; Syntax: pointerLine:=DllCall(dll_path "\<b>addScript</b>","Str","MsgBox % A_Now"[,"Uchar",Execute,],"Cdecl UPTR")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   pointerLine - addScript() returns a pointer to the first line in the late included script.
;	   ("MsgBox...") - script as text(or variable containing text) added to the running script.
;	   Execute - 0 = add only but do not execute.<br>1 = add script and start execution.<br>2 = add script and wait until executed
; Return Value:
;     <b>Cdecl UInt</b> - return value is a pointer to the first line of new created lines.
; Remarks:
;		Line pointer can be used in ahkExecuteLine() to execute one line only or until a return is encountered.
; Related:
; Example:
;		file:Example_addScript.ahk
;

;
; Function: ahkExec
; Description:
;      ahkExec is used to run some code from text/variable temporarily.
; Syntax: success:=DllCall(dll_path "\<b>ahkExec</b>","Str","MsgBox % A_Now")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   success - ahkExec() returns true if script was executed and false if not
;	   ("MsgBox...") - script as text(or variable containing text) that will be executed.
; Return Value:
;     Returns true if script was executed and false if there was an error.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_ahkExec.ahk
;

;
; Function: ahkLabel
; Description:
;      ahkLabel is used to launch a Goto/GoSub routine in script.
; Syntax: labelFound:=DllCall(dll_path "\<b>ahkLabel</b>","Str","MyLabel","UInt",nowait,"Cdecl UInt")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   labelFound - ahkLabel() returns 1 if label exists else 0.
;	   label ("MyLabel") - name of label to launch.
;	   nowait - 0 = Gosub (default), 1 = GoTo	(PostMessage mode)
; Return Value:
;     <b>Cdecl UInt</b> - returns 1 if label exists else 0.
; Remarks:
;		Default is 0 = wait for code to finish execution, this is because PostMessage is not reliable and might be scipped so script will not execute.
; Related:
; Example:
;		file:Example_ahkLabel.ahk
;

;
; Function: ahkFunction
; Description:
;      ahkFunction is used to launch a function in script.
; Syntax: ReturnValue:=DllCall(dll_path "\<b>ahkFunction</b>","Str","MyFunction",["Str",param1,"Str",param2,...,param10],"Cdecl Str")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   ReturnValue - ahkFunction returns the value that is returned by the launched function as string/text.
;	   param1 to param10 - Parameters to pass to the function. These parameters are optional.
; Return Value:
;     <b>Cdecl Str</b> - return value is always a string/text, add 0 to make sure it resolves to number if necessary.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_ahkFunction.ahk
;

;
; Function: ahkPostFunction
; Description:
;      ahkPostFunction is used to launch a function in script and return <b>immediately</b> (so function will run in background and return value will be ignored).
; Syntax: ReturnValue:=DllCall(dll_path "\<b>ahkPostFunction</b>","Str","MyFunction",["Str",param1,"Str",param2,...,param10],"Cdecl UInt")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   ReturnValue - ahkPostFunction returns 1 if function exists else 0.
;	   param1 to param10 - Parameters to pass to the function. These parameters are optional.
; Return Value:
;     <b>Cdecl UInt</b> - return returns 0 if function exists else -1.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_ahkPostFunction.ahk
;

;
; Function: ahkassign
; Description:
;      ahkassign is used to assign a string to a variable in script.
; Syntax: Success:=DllCall(dll_path "\<b>ahkassign</b>","Str","variable","Str",value)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   ReturnValue - returns 0 on success and -1 on failure.
;	   variable - name of variable to save value in.
;	   value - value to be saved in variable, can be sting/text only.
; Return Value:
;     Return value is 0 on success and -1 on failure.
; Remarks:
;		ahkassign will create the variable if it does not exist.
; Related:
; Example:
;		file:Example_ahkassign.ahk
;

;
; Function: ahkgetvar
; Description:
;      ahkgetvar is used to get a value from a variable in script.
; Syntax: Value:=DllCall(dll_path "\<b>ahkgetvar</b>","Str","var","UInt",getPointer,"Cdecl Str")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   Value - ahkgetvar() returns the value of a variable as string.
;	   var - name of variable to get value from.
;	   getPointer - set to 1 to get pointer of variable to use with Alias(), else 0 to get the value.
; Return Value:
;     <b>Cdecl Str</b> - return value is always a string, add 0 to convert to integer if necessary, especially when using getPointer.
; Remarks:
;		ahkgetvar returns empty if variable does not exist or is empty.
; Related:
; Example:
;		file:Example_ahkgetvar.ahk
;

;
; Function: ahkTerminate
; Description:
;      ahkTerminate is used to stop and exit a running script in dll. ( Available in AutoHotkey[Mini].dll only, not in AutoHotkey_H.exe)
; Syntax: DllCall(dll_path "\<b>ahkTerminate</b>","Int",timeout,"Cdecl Int")
; Parameters:
;	   timeout - Time in miliseconds to wait until thread exits.<br>If this parameter is 0 it defaults to 500.
; Return Value:
;     Returns always 0.
; Remarks:
;		ahkTerminate will destroy script hotkeys and hotstrings and exit the thread. ahkTerminate uses SendMessageTimeout for better reliability.
; Related:
; Example:
;		file:Example_ahkTerminate.ahk
;

;
; Function: ahkReload
; Description:
;      ahkReload is used to stop and exit a running script in dll. ( Available in AutoHotkey[Mini].dll only, not in AutoHotkey_H.exe )
; Syntax: DllCall(dll_path "\<b>ahkReload</b>")
; Parameters:
;	   No parameters - No parameters
; Return Value:
;     Returns always 0 for success.
; Remarks:
;		ahkReload will destroy hotkeys and hotstrings, exit thread and start it again using same parameters/file/text.
; Related:
; Example:
;		file:Example_ahkReload.ahk
;

;
; Function: ahkFindFunc
; Description:
;      ahkFundFunc is used to get function its pointer. (Equivalent to FindFunc)
; Syntax: DllCall(dll_path "\<b>ahkFindFunc</b>","Str","func","Cdecl PTR")
; Parameters:
;	   func - Name of function.
; Return Value:
;     <b>Cdecl UPTR</b> - Function pointer.
; Remarks:
;		None
; Related:
; Example:
;		file:Example_ahkFindFunc.ahk
;

;
; Function: ahkFindLabel
; Description:
;      ahkFundLabel is used to get a pointer to the label. (Equivalent to FindLabel)
; Syntax: DllCall(dll_path "\<b>ahkFindLabel</b>","Str","label","Cdecl UPTR")
; Parameters:
;	   label - Name of label.
; Return Value:
;     <b>Cdecl UPTR</b> - Label pointer.
; Remarks:
;		None
; Related:
; Example:
;		file:Example_ahkFindLabel.ahk
;

;
; Function: ahkPause
; Description:
;      ahkPause is used to pause a thread and run traditional AutoHotkey Sleep, so it is different to DllCall("SuspendThread",...), so thread remains active and you can still do various things with it like ahkgetvar.
; Syntax: DllCall(dll_path "\<b>ahkPause</b>"[,"Str","on"])
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   State - On or 1 to enable Pause / Off or 0 to disable pause. Pass empty string to find out whether thread is paused.
; Return Value:
;     <b>Cdecl Int</b> - return 1 if thread is paused or 0 if it is not.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_ahkPause.ahk
;

;
; Function: ahkExecuteLine
; Description:
;      ahkExecuteLine is used to execute the script from previously added line via addScript or addFile.
; Syntax: pointerNextLine := DllCall(dll_path "\<b>ahkExecuteLine</b>"[,"UPTR",pointerLine,"UInt",mode,"Uint",wait],"Cdecl UPTR")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   pointerNextLine - a pointer to next line. When no line pointer is passed, it returns a pointer to FirstLine.
;	   pointerLine - a pointer to the line to be executed. 
;	   mode - 0 = will not execute but return a pointer to next line.<br>1 = UNTIL_RETURN<br>2 = UNTIL_BLOCK_END<br>3 = ONLY_ONE_LINE
;	   wait - set 1 to wait until the execution finished (be careful as it may happen that the dll call never return!
; Return Value:
;     <b>Cdecl UInt</b> - if no pointerLine is passed it returns a pointer to FirstLine, else it returns a pointer to NextLine.
; Remarks:
;		addFile and addScript return a line pointer, so you can add one or more lines of script and launch them without having a Label to it.<br><b>A_LineFile<b/> contains the full file path of currently running script (ahkdll + addFile) or the script in text form (ahktextdll or addScript).<br>Functions are skipped so you recive the pointer to line after function.
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: Alias
; Description:
;      Build in function Alias is used to convert a variable into a pointer to another variable.
; Syntax: Alias(alias, alias_for)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   alias - A reference to a local variable or function parameter.
;	   alias_for - A reference to or the address of a user-defined variable.
; Return Value:
;     Not used.
; Remarks:
;		An alias is akin to a ByRef function parameter. However, an optional ByRef parameter is not an alias when it is omitted from a function call.
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: cacheEnable
; Description:
;      Build in function to re-enable binary number caching for a variable.
; Syntax: cacheEnable(var)<br>cacheEnable(%varContainingNameOfVar%)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   var - An unquoted variable reference.
; Return Value:
;     Not used.
; Remarks:
;		In AutoHotkey v1.0.48, binary numbers are cached in variables to avoid conversions to/from strings. However, taking the address of a variable (e.g. &var) marks that variable as uncacheable because it allows the user to read/write its contents in unconventional ways. This function may be used to re-enable binary number caching however, any address previously taken may become invalid or out-of-sync with the actual contents of the variable.<br><br>If var is a ByRef parameter, caching is re-enabled for the underlying variable.
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: FindFunc
; Description:
;      Build in function to retrieve a pointer to the Func structure of a function with the given name.
; Syntax: func_ptr := FindFunc(FuncName)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   FuncName - The name of a user-defined or built-in function.
; Return Value:
;     Pointer to a function.
; Remarks:
;		None
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: FindLabel
; Description:
;      Build in function to retrieve a pointer to structure of a label.
; Syntax: label_ptr := FindLabel(LabelName)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   LabelName - The name of an existing label.
; Return Value:
;     Pointer to a label.
; Remarks:
;		None
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: getTokenValue
; Description:
;      Build in function to retrieve a value from an ExprTokenType structure.
; Syntax: value := getTokenValue(pointer-expression)<br>value := getTokenValue(varContainingPointer+0)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   pointer-expression - An expression which results in a pointer to an ExprTokenType structure. This must not be a variable reference, string, floating-point number or numeric literal.
; Return Value:
;     Value from an ExprTokenType structure.
; Remarks:
;		None
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: getVar
; Description:
;      Build in function to get a pointer to the Var structure of a user-defined variable.
; Syntax: var_ptr := getVar(var)<br>var_ptr := getVar(%varContainingNameOfVar%)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   var - A reference to a user-defined variable.
; Return Value:
;     Value from an ExprTokenType structure.
; Remarks:
;		None
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: Static
; Description:
;      Build in function to make a variable static.
; Syntax: static(var)<br>static(%varContainingNameOfVar%)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   var - A reference to a local variable or non-ByRef parameter.
; Return Value:
;     Value from an ExprTokenType structure.
; Remarks:
;		None
; Related:
; Example:
;		file: AhkDll_Example.ahk
;

;
; Function: AutoHotkeyMini
; Description:
;      AutoHotkeyMini.dll excludes many commands to support much faster load/reload and less memory consumption.
; Syntax: Useful when a program uses many threads.
; Parameters:
;	   Disabled commands - Hotkey (as well as Hotkey + Hotstring functionality), Gui…, GuiControl[Get], Menu..., TrayIcon, FileInstall, ListVars, ListLines, ListHotkeys, KeyHistory, SplashImage, SplashText..., Progress, PixelSearch, PixelGetColor, InputBox, FileSelect and FolderSelect dialogs, Input, BlockInput MouseMove[Off], Build in variables related to Hotkeys and Icons as well as Gui, A_ThisHotkey..., A_IsSuspended, A_Icon....
; Return Value:
;     None.
; Remarks:
;		None.
; Related:
; Example:
;		file: No_Example.ahk
;

;
; Function: DynaCall
; Description:
;      Build in function, similar to DllCall but works with DllCall structures and Object syntax.<br>It is faster than DllCall, easier to use and saves a lot of typing and code.
; Syntax: objfunc:=DynaCall(FilePath . "\function",[parameter_definition,default_param1,default_param2,...])<br><br>objfunc.[DllCallReturnType](params...)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   function - Can be a function address, name of function or path\name for the function.<br>E.g. "GetProcAddress" or "kernel32\LoadLibrary"
;	   parameter_definition - Int = i / Str = s / AStr = a / WStr = w / Short = h / Char = c / Float = f / Double = d / Ptr = t / Int64 = 6 <br>U prefix and * or p is supported as well, for example: "ui=ui*s"<br>You can change order of parameters using an object declaration.<br>For example: DynaCall("SendMessage",["t=tuitt",3,2],hwnd,WM_ACTIVATE:=0x06)<br>So when calling the function first parameter is wParam and second Msg. See example below.
;	   default_param - You can set the parameters already when a Token is created, that way you can call the function without passing parameters as these are alredy set. When default_value is not given, parameters will default to 0 or "".<br>CDecl calling convention can be changed using ==, for example "t==ui".
;	   DllCall_Return_Type - Optional. Using this you can override default return type previously defined in parameter_definition.<br>Use same return types as in DllCall (Int/Str/AStr/WStr/Short/Char/Float/Double/Ptr/Int64).
;	   How to call the function - Any Object syntax can be used to call the function. For example:<br><br>objfunc[params...]<br>objfunc.param<br>objfunc.(params...) ; NOTE it is not objfunc(params...)<br>objfunc.param1 := param2<br>objfunc.Str(params...) ; here we can use a different return type than defined in defnition.
; Return Value:
;     objfunc will contain the DynaToken object that can be used to call the dll function using any object syntax.
; Remarks:
;		DynaCall uses same ErrorLevel as DllCall.<br>DynaCall can be also used to call the function, therefore pass a DynaToken that was created via DynaCall previously, followed by parameters. E.g. DynaCall(objfunc,parameters...).
; Related:
; Example:
;		file:Example_DynaCall.ahk
;

;
; Function: CriticalSection
; Description:
;      Build in functions, to Lock and Unlock a CriticalSection.
; Syntax: Lock(&CriticalSeciton) / TryLock(&CriticalSeciton) / UnLock(&CriticalSeciton)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   &CriticalSection - Pointer to a critical section, see: <a href=http://www.autohotkey.com/forum/viewtopic.php?t=51335>CriticalSection</href>.
; Return Value:
;     None.
; Remarks:
;		These functions are build in because they are faster than user defined functions. CriticalSection function can be found <a href=http://www.autohotkey.com/forum/topic51335.html>here</a>.
; Related:
; Example:
;		file: No_Example.ahk
;


;
; Function: CriticalObject
; Description:
;      Build in function to make an object multi-thread safe.
; Syntax: CriticalObject := CriticalObject([obj,lpCriticalSection])
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   obj - Can be an object or an object reference, when omitted an empty object will be created.<br>To find out the pointer of referenced object or Critical Section this must be a CriticalObject, see example.
;	   lpCriticalSection - a pointer to Critical Section, when omitted AutoHotkey will create internal Critical Section and use it.<br>When obj is a CriticalObject this can be 1 to get pointer to referenced object or 2 to get pointer to Critical Section.
; Return Value:
;     Returns a Critical Object. This object uses Critical Section internally to disallow simultanous access while in script it can be used as a normal object
; Remarks:
;		Note, when CriticalObject is deleted, its CrticalSection is not deleted, if needed delete it manually before deleting last reference to CriticalObject.<br>Note, you cannot use ObjRelease(CriticalObject) because it will not give you the referenced object but the criticalObject.
; Related:
; Example:
;		file:Example_CriticalObject.ahk
;


;
; Function: MemoryLoadLibrary
; Description:
;      Build in function to load a dll multiple times from Memory.<br>Based on http://www.joachim-bauch.de/tutorials/load_dll_memory.html
; Syntax: hLibrary:=MemoryLoadLibrary(FilePath)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   FilePath - A path to a file on disk that will be loaded into memory and LoadLibrary from there.
; Return Value:
;     Returns handle of Library.
; Remarks:
;		This handle can be used to get process address of exported functions for this dll (MemoryGetProcAddress).
; Related:
; Example:
;		file:Example_MemoryLibrary.ahk
;

;
; Function: ResourceLoadLibrary
; Description:
;      Build in function to load a dll multiple times from Resource. (For compiled scripts only) <br>Based on http://www.joachim-bauch.de/tutorials/load_dll_memory.html
; Syntax: hLibrary:=ResourceLoadLibrary(FilePath)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   FilePath - A path to a file that was included via FileInstall,FilePath,....
; Return Value:
;     Returns handle of Library.
; Remarks:
;		This handle can be used to get process address of exported functions for this dll (MemoryGetProcAddress).
; Related:
; Example:
;		file:Example_ResourceLibrary.ahk
;

;
; Function: MemoryGetProcAddress
; Description:
;      Build in function to get a pointer to exported functions of a dll to use in DllCall().<br>Based on http://www.joachim-bauch.de/tutorials/load_dll_memory.html
; Syntax: pFunc:=MemoryGetProcAddress(hLibrary,"function")
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   hLibrary - The handle returned by MemoryLoadLibrary().
;	   function - Function to get the pointer for.
; Return Value:
;     Returns a pointer to the function that can be used in DllCall().
; Remarks:
;		You can access these functions only by using pointer in DllCall!
; Related:
; Example:
;		file:Example_MemoryLibrary.ahk
;

;
; Function: MemoryFreeLibrary
; Description:
;      Build in function to free the Library loaded from Memory by MemoryLoadLibrary.<br>Based on http://www.joachim-bauch.de/tutorials/load_dll_memory.html
; Syntax: MemoryFreeLibrary(hLibrary)
; Parameters:
;	   <b>Parameter</b> - <b>Description</b>
;	   hLibrary - The handle returned by MemoryLoadLibrary().
; Return Value:
;     This function does not return a value.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_MemoryLibrary.ahk
;

;
; Function: Other Changes
; Description:
;      Differences to AutoHotkey_L.exe and other changes.
; Syntax: General Changes
; Parameters:
;	   <b>Change</b> - <b>Info</b>
;	   #NoEnv - this is now default, use #NoEnv, Off to enable use of Environment variables.
;	   Sleep in Send - Send {100}a{100}b including integer higher than 9 in brackets will do a sleep between sending keystrokes
;	   SendMode Input - this is now default SendMode, use SendMode,... to change.
;	   A_DllPath - Build in variable that contains the path of running dll.
;	   StdLib - additionally Lib.lnk (a link to library folder) can be present in AutoHotkey.exe directory.
;	   Input - added support for Multithreading.<br>Added A option to append input to the variable (so variable will not be set empty before input starts)
;	   Compile exe or dll - in AutoHotkey_H any bin/exe/dll can be compiled. Modified Ahk2Exe can be found here: https://github.com/HotKeyIt/Ahk2Exe
;	   /E switch - Compiled exe and dll can stil execute scripts.<br>Run myexe.exe /E myscript.ahk<br>dll.ahkdll("myscript.ahk","","/E")<br>dll.ahktextdll("MsgBox","","/E")
; Return Value:
;     None.
; Remarks:
;		None.
; Related:
; Example:
;		file:Example_Input.ahk
;