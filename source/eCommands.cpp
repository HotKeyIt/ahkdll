/*
EXPORT int EStringSplit(unsigned char aActionType, char *aArrayName, char *aInputString, char *aDelimiterList, char *aOmitList){
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->StringSplit(aArrayName, aInputString, aDelimiterList, aOmitList);
return 0;
}
EXPORT int ESplitPath(unsigned char aActionType, char *aFileSpec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->SplitPath(aFileSpec);
 return 0;
}
EXPORT int EPerformSort(unsigned char aActionType, char *aContents, char *aOptions)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->PerformSort(aContents, aOptions);
 return 0;
}
EXPORT int EGetKeyJoyState(unsigned char aActionType, char *aKeyName, char *aOption)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->GetKeyJoyState(aKeyName, aOption);
 return 0;
}
EXPORT int EDriveSpace(unsigned char aActionType, char *aPath, bool aGetFreeSpace)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->DriveSpace(aPath,  aGetFreeSpace);
 return 0;
}
EXPORT int EDrive(unsigned char aActionType, char *aCmd, char *aValue, char *aValue2)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->Drive(aCmd, aValue, aValue2);
 return 0;
}
EXPORT int EDriveLock(unsigned char aActionType, char aDriveLetter, bool aLockIt)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->DriveLock(aDriveLetter,  aLockIt);
 return 0;
}
EXPORT int EDriveGet(unsigned char aActionType, char *aCmd, char *aValue)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->DriveGet(aCmd, aValue);
 return 0;
}
EXPORT int ESoundPlay(unsigned char aActionType, char *aFilespec, bool aSleepUntilDone)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->SoundPlay(aFilespec,  aSleepUntilDone);
 return 0;
}
EXPORT int EURLDownloadToFile(unsigned char aActionType, char *aURL, char *aFilespec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->URLDownloadToFile(aURL, aFilespec);
 return 0;
}
EXPORT int EFileSelectFile(unsigned char aActionType, char *aOptions, char *aWorkingDir, char *aGreeting, char *aFilter)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileSelectFile(aOptions, aWorkingDir, aGreeting, aFilter);
 return 0;
}
EXPORT int EFileSelectFolder(unsigned char aActionType, char *aRootDir, char *aOptions, char *aGreeting)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileSelectFolder(aRootDir, aOptions, aGreeting);
 return 0;
}
EXPORT int EFileGetShortcut(unsigned char aActionType, char *aShortcutFile)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileGetShortcut(aShortcutFile);
 return 0;
}
EXPORT int EFileCreateShortcut(unsigned char aActionType, char *aTargetFile, char *aShortcutFile, char *aWorkingDir, char *aArgs, char *aDescription, char *aIconFile, char *aHotkey, char *aIconNumber, char *aRunState)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileCreateShortcut(aTargetFile, aShortcutFile, aWorkingDir, aArgs, aDescription, aIconFile, aHotkey, aIconNumber, aRunState);
 return 0;
}
EXPORT int EFileCreateDir(unsigned char aActionType, char *aDirSpec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileCreateDir(aDirSpec);
 return 0;
}
EXPORT int EFileRead(unsigned char aActionType, char *aFilespec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileRead(aFilespec);
 return 0;
}
EXPORT int EFileReadLine(unsigned char aActionType, char *aFilespec, char *aLineNumber)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileReadLine(aFilespec, aLineNumber);
 return 0;
}
EXPORT int EWriteClipboardToFile(unsigned char aActionType, char *aFilespec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WriteClipboardToFile(aFilespec);
 return 0;
}
EXPORT int EFileDelete(unsigned char aActionType)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileDelete();
 return 0;
}
EXPORT int EFileRecycle(unsigned char aActionType, char *aFilePattern)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileRecycle(aFilePattern);
 return 0;
}
EXPORT int EFileRecycleEmpty(unsigned char aActionType, char *aDriveLetter)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileRecycleEmpty(aDriveLetter);
 return 0;
}
EXPORT int EFileInstall(unsigned char aActionType, char *aSource, char *aDest, char *aFlag)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileInstall(aSource, aDest, aFlag);
 return 0;
}
EXPORT int EFileGetAttrib(unsigned char aActionType, char *aFilespec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileGetAttrib(aFilespec);
 return 0;
}
EXPORT int EFileGetTime(unsigned char aActionType, char *aFilespec, char aWhichTime)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileGetTime(aFilespec, aWhichTime);
 return 0;
}
EXPORT int EFileGetSize(unsigned char aActionType, char *aFilespec, char *aGranularity)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileGetSize(aFilespec, aGranularity);
 return 0;
}
EXPORT int EFileGetVersion(unsigned char aActionType, char *aFilespec)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->FileGetVersion(aFilespec);
 return 0;
}
EXPORT int EIniRead(unsigned char aActionType, char *aFilespec, char *aSection, char *aKey, char *aDefault)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->IniRead(aFilespec, aSection, aKey, aDefault);
 return 0;
}
EXPORT int EIniWrite(unsigned char aActionType, char *aValue, char *aFilespec, char *aSection, char *aKey)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->IniWrite(aValue, aFilespec, aSection, aKey);
 return 0;
}
EXPORT int EIniDelete(unsigned char aActionType, char *aFilespec, char *aSection, char *aKey)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->IniDelete(aFilespec, aSection, aKey);
 return 0;
}
EXPORT int ERegRead(unsigned char aActionType, HKEY aRootKey, char *aRegSubkey, char *aValueName)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->RegRead(aRootKey, aRegSubkey, aValueName);
 return 0;
}
EXPORT int ERegWrite(unsigned char aActionType, DWORD aValueType, HKEY aRootKey, char *aRegSubkey, char *aValueName, char *aValue)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->RegWrite(aValueType, aRootKey, aRegSubkey, aValueName, aValue);
 return 0;
}
EXPORT int ESplashTextOn(unsigned char aActionType, int aWidth, int aHeight, char *aTitle, char *aText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->SplashTextOn( aWidth,  aHeight, aTitle, aText);
 return 0;
}
EXPORT int EToolTip(unsigned char aActionType, char *aText, char *aX, char *aY, char *aID)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ToolTip(aText, aX, aY, aID);
 return 0;
}
EXPORT int ETrayTip(unsigned char aActionType, char *aTitle, char *aText, char *aTimeout, char *aOptions)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->TrayTip(aTitle, aText, aTimeout, aOptions);
 return 0;
}
EXPORT int ETransform(unsigned char aActionType, char *aCmd, char *aValue1, char *aValue2)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->Transform(aCmd, aValue1, aValue2);
 return 0;
}
EXPORT int EInput(unsigned char aActionType)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->Input(); // The Input command.
 return 0;
}
EXPORT int EWinMove(unsigned char aActionType, char *aTitle, char *aText, char *aX, char *aY
		, char *aWidth , char *aHeight , char *aExcludeTitle , char *aExcludeText )
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinMove(aTitle, aText, aX, aY
		, aWidth , aHeight , aExcludeTitle , aExcludeText );
 return 0;
}
EXPORT int EWinMenuSelectItem(unsigned char aActionType, char *aTitle, char *aText, char *aMenu1, char *aMenu2
		, char *aMenu3, char *aMenu4, char *aMenu5, char *aMenu6, char *aMenu7
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinMenuSelectItem(aTitle, aText, aMenu1, aMenu2
		, aMenu3, aMenu4, aMenu5, aMenu6, aMenu7
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlSend(unsigned char aActionType, char *aControl, char *aKeysToSend, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText, bool aSendRaw)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlSend(aControl, aKeysToSend, aTitle, aText
		, aExcludeTitle, aExcludeText,  aSendRaw);
 return 0;
}
EXPORT int EControlClick(unsigned char aActionType, vk_type aVK, int aClickCount, char *aOptions, char *aControl
		, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlClick(aVK,  aClickCount, aOptions, aControl
		, aTitle, aText, aExcludeTitle, aExcludeText);
return 0;
}
EXPORT int EControlMove(unsigned char aActionType, char *aControl, char *aX, char *aY, char *aWidth, char *aHeight
		, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlMove(aControl, aX, aY, aWidth, aHeight
		, aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlGetPos(unsigned char aActionType, char *aControl, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlGetPos(aControl, aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlGetFocus(unsigned char aActionType, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlGetFocus(aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlFocus(unsigned char aActionType, char *aControl, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlFocus(aControl, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlSetText(unsigned char aActionType, char *aControl, char *aNewText, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlSetText(aControl, aNewText, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlGetText(unsigned char aActionType, char *aControl, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlGetText(aControl, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControl(unsigned char aActionType, char *aCmd, char *aValue, char *aControl, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->Control(aCmd, aValue, aControl, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EControlGet(unsigned char aActionType, char *aCommand, char *aValue, char *aControl, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ControlGet(aCommand, aValue, aControl, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EGuiControl(unsigned char aActionType, char *aCommand, char *aControlID, char *aParam3)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->GuiControl(aCommand, aControlID, aParam3);
 return 0;
}
EXPORT int EGuiControlGet(unsigned char aActionType, char *aCommand, char *aControlID, char *aParam3)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->GuiControlGet(aCommand, aControlID, aParam3);
 return 0;
}
EXPORT int EStatusBarGetText(unsigned char aActionType, char *aPart, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->StatusBarGetText(aPart, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EStatusBarWait(unsigned char aActionType, char *aTextToWaitFor, char *aSeconds, char *aPart, char *aTitle, char *aText
		, char *aInterval, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->StatusBarWait(aTextToWaitFor, aSeconds, aPart, aTitle, aText
		, aInterval, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EScriptPostSendMessage(unsigned char aActionType, bool aUseSend)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ScriptPostSendMessage( aUseSend);
 return 0;
}
EXPORT int EScriptProcess(unsigned char aActionType, char *aCmd, char *aProcess, char *aParam3)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->ScriptProcess(aCmd, aProcess, aParam3);
 return 0;
}
EXPORT int EWinSet(unsigned char aActionType, char *aAttrib, char *aValue, char *aTitle, char *aText
		, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinSet(aAttrib, aValue, aTitle, aText
		, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EWinSetTitle(unsigned char aActionType, char *aTitle, char *aText, char *aNewTitle
		, char *aExcludeTitle , char *aExcludeText )
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinSetTitle(aTitle, aText, aNewTitle
		, aExcludeTitle , aExcludeText );
 return 0;
}
EXPORT int EWinGetTitle(unsigned char aActionType, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinGetTitle(aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EWinGetClass(unsigned char aActionType, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinGetClass(aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EWinGet(unsigned char aActionType, char *aCmd, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinGet(aCmd, aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EWinGetText(unsigned char aActionType, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinGetText(aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EWinGetPos(unsigned char aActionType, char *aTitle, char *aText, char *aExcludeTitle, char *aExcludeText)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->WinGetPos(aTitle, aText, aExcludeTitle, aExcludeText);
 return 0;
}
EXPORT int EEnvGet(unsigned char aActionType, char *aEnvVarName)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->EnvGet(aEnvVarName);
 return 0;
}
EXPORT int ESysGet(unsigned char aActionType, char *aCmd, char *aValue)
{
ArgStruct *new_arg = (ArgStruct *)SimpleHeap::Malloc(20 * sizeof(ArgStruct)); 
Line *tempLine = new Line(1, 5, aActionType, new_arg, 20);
tempLine->SysGet(aCmd, aValue);
 return 0;
}


*/
