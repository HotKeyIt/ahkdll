/*
SetWinDelay,-1
OnExit, GuiClose

WatchFolders:="C:\Temp*|%A_Temp%*|%A_Desktop%|%A_DesktopCommon%|%A_MyDocuments%*|%A_ScriptDir%|%A_WinDir%*"
Gui,+Resize
Gui,Add,ListView,r10 w800 vWatchingDirectoriesList HWNDhList1 gShow,WatchingDirectories|WatchingSubdirs
LoopParse,%WatchFolders%,|
   WatchDirectory(A_LoopField,"ReportChanges")
   ,LV_Add("",SubStr(A_LoopField,0)="*" ? (SubStr(A_LoopField,1,StrLen(A_LoopField)-1)) : A_LoopField
          ,SubStr(A_LoopField,0)="*" ? 1 : 0)
LV_ModifyCol(1,"AutoHdr")
Gui,Add,ListView,r30 w800 vChangesList HWNDhList2 gShow,Time|FileChangedFrom - Double click to show in Explorer|FileChangedTo - Double click to show in Explorer
Gui,Add,Button,gAdd Default,Watch new directory
Gui,Add,Button,gDelete x+1,Stop watching all directories
Gui,Add,Button,gClear x+1,Clear List
Gui,Add,StatusBar,,Changes Registered
Gui, Show

Return

Clear:
Gui,ListView, ChangesList
LV_Delete()
Return

Delete:
WatchDirectory("")
Gui,ListView, WatchingDirectoriesList
LV_Delete()
Gui,ListView, ChangesList
TotalChanges:=0
SB_SetText("Changes Registered")
Return

Show:
If A_GuiEvent!=DoubleClick
   Return
Gui,ListView,%A_GuiControl%
LV_GetText(file,A_EventInfo,2)
If file=
   LV_GetText(file,A_EventInfo,3)
Run,% "explorer.exe /e`, /n`, /select`," . file
Return

Add:
Gui,+OwnDialogs
dir:=""
FileSelectFolder,dir,,3,Select directory to watch for
If !dir
   Return
SetTimer,SetMsgBoxButtons,-10
MsgBox, 262146,Add directory,Would you like to watch for changes in:`n%dir%

Gui,ListView, WatchingDirectoriesList
If (A_MsgBoxResult="Retry")
   WatchDirectory(dir "*"),LV_Add("",dir,1)
If (A_MsgBoxResult="Ignore")
   WatchDirectory(dir),LV_Add("",dir,0)
LV_ModifyCol(1,"AutoHdr")
Gui,ListView, ChangesList
Return

SetMsgBoxButtons:
   WinWait, ahk_class #32770
   WinActivate
   WinWaitActive
   ControlSetText,Button2,&Incl. subdirs, ahk_class #32770
   ControlSetText,Button3,&Excl. subdirs, ahk_class #32770
Return

ReportChanges(this,from,to){
   global TotalChanges
   Gui,ListView, ChangesList
   LV_Insert(1,"",A_Hour ":" A_Min ":" A_Sec ":" A_MSec,from,to)
   LV_ModifyCol()
   LV_ModifyCol(1,"AutoHdr"),LV_ModifyCol(2,"AutoHdr")
   TotalChanges++
   SB_SetText("Changes Registered " . TotalChanges)
}

GuiClose:
WatchDirectory("") ;Stop Watching Directory = delete all directories
ExitApp
*/
;~ #include <Struct>
WatchDirectory(p*){
  ;Structures
  static FILE_NOTIFY_INFORMATION:="DWORD NextEntryOffset,DWORD Action,DWORD FileNameLength,WCHAR FileName[1]"
  static OVERLAPPED:="ULONG_PTR Internal,ULONG_PTR InternalHigh,{struct{DWORD offset,DWORD offsetHigh},PVOID Pointer},HANDLE hEvent"
  ;Variables
  static running,sizeof_FNI:=65536,temp1:=VarSetCapacity(nReadLen,8),WatchDirectory:=RegisterCallback("WatchDirectory","F",0,0)
  static timer,ReportToFunction,LP,temp2:=VarSetCapacity(LP,(260)*(A_PtrSize/2),0)
  static __:=Object(),reconnect:=Object(),_:=Object(),DirEvents,StringToRegEx:="\\\|.\.|+\+|[\[|{\{|(\(|)\)|^\^|$\$|?\.?|*.*"
  ;ReadDirectoryChanges related
        ,FILE_NOTIFY_CHANGE_FILE_NAME:=0x1,FILE_NOTIFY_CHANGE_DIR_NAME:=0x2,FILE_NOTIFY_CHANGE_ATTRIBUTES:=0x4
        ,FILE_NOTIFY_CHANGE_SIZE:=0x8,FILE_NOTIFY_CHANGE_LAST_WRITE:=0x10,FILE_NOTIFY_CHANGE_CREATION:=0x40
        ,FILE_NOTIFY_CHANGE_SECURITY:=0x100
        ,FILE_ACTION_ADDED:=1,FILE_ACTION_REMOVED:=2,FILE_ACTION_MODIFIED:=3
        ,FILE_ACTION_RENAMED_OLD_NAME:=4,FILE_ACTION_RENAMED_NEW_NAME:=5
        ,OPEN_EXISTING:=3,FILE_FLAG_BACKUP_SEMANTICS:=0x2000000,FILE_FLAG_OVERLAPPED:=0x40000000
        ,FILE_SHARE_DELETE:=4,FILE_SHARE_WRITE:=2,FILE_SHARE_READ:=1,FILE_LIST_DIRECTORY:=1
  If IsObject(p) && max:=p.Length(){
    If (max=1 && p.1=""){
      for i,folder in _
        DllCall("CloseHandle","Uint",__[folder].hD),DllCall("CloseHandle","Uint",__[folder].O.hEvent)
        ,__.Remove(folder)
      _:=[]
      ,DirEvents:=Struct("HANDLE[1000]")
      ,DllCall("KillTimer","Uint",0,"Uint",timer)
      ,timer:=""
      Return 0
    } else {
      ReportToFunction:=p.2
      ; If !IsFunc(ReportToFunction)
        ; Return -1 ;DllCall("MessageBox","Uint",0,"Str","Function " ReportToFunction " does not exist","Str","Error Missing Function","UInt",0)
      RegExMatch(p.1,"^([^/\*\?<>\|`"]+)(\*)?(\|.+)?$",dir)
      loop % dir.count()
        dir%A_Index%:=dir[A_Index]
      if (SubStr(dir1,-1)="\")
        dir1:=SubStr(dir1,1,-1)
      dir3:=SubStr(dir3,2)
      If (max=2 && ReportToFunction=""){
        for i,folder in _
          If (dir1=SubStr(folder,1,-1))
            Return 0 ,DirEvents[i]:=DirEvents[_.Length()],DirEvents[_.Length()]:=0
                   __.Remove(folder),_[i]:=_[_.Length()],_.Remove(i)
        Return 0
      }
    }
    if !InStr(FileExist(dir1),"D")
      Return -1 ;DllCall("MessageBox","Uint",0,"Str","Folder " dir1 " does not exist","Str","Error Missing File","UInt",0)
    for i,folder in _
    {
      If (dir1=SubStr(folder,1,-1) || (InStr(dir1,folder) && __[folder].sD))
        Return 0
      else if (InStr(SubStr(folder,1,-1),dir1 "\") && dir2){ ; replace watch
        DllCall("CloseHandle","PTR",__[folder].hD),DllCall("CloseHandle","PTR",__[folder].O.hEvent),reset:=i
      }
    }
    LP:=SubStr(LP,1,DllCall("GetLongPathName","Str",dir1,"STR",LP,"Uint",VarSetCapacity(LP))) "\"
    If !(reset && __[reset]:=LP)
      _.Push(LP)
    __[LP,"dir"]:=LP
    __[LP].hD:=DllCall("CreateFile","Str",StrLen(LP)=3?SubStr(LP,1,2):LP,"UInt",0x1,"UInt",0x1|0x2|0x4
              ,"PTR",0,"UInt",0x3,"UInt",0x2000000|0x40000000,"PTR",0)
    __[LP].sD:=(dir2=""?0:1)

    LoopParse,%StringToRegEx%,|
      StrReplace,dir3,%dir3%,% SubStr(A_LoopField,1,1),% SubStr(A_LoopField,2)
    StrReplace,dir3,%dir3%,%A_Space%,\s
    LoopParse,%dir3%,|
    {
      If A_Index=1
        dir3:=""
      pre:=(SubStr(A_LoopField,1,2)="\\"?2:0)
      succ:=(SubStr(A_LoopField,-2)="\\"?2:0)
      dir3.=(dir3?"|":"") (pre?"\\\K":"")
          . SubStr(A_LoopField,1+pre,StrLen(A_LoopField)-pre-succ)
          . ((!succ && !InStr(SubStr(A_LoopField,1+pre,StrLen(A_LoopField)-pre-succ),"\"))?"[^\\]*$":"") (succ?"$":"")
    }
    __[LP].FLT:="i)" dir3
    __[LP].FUNC:=ReportToFunction
    __[LP].CNG:=(p.3?p.3:(0x1|0x2|0x4|0x8|0x10|0x40|0x100))
    If !reset {
      __[LP].SetCapacity("pFNI",sizeof_FNI)
      __[LP].FNI:=Struct(FILE_NOTIFY_INFORMATION,__[LP].GetAddress("pFNI"))
      __[LP].O:=Struct(OVERLAPPED)
    }
    __[LP].O.hEvent:=DllCall("CreateEvent","Uint",0,"Int",1,"Int",0,"UInt",0)
    If (!DirEvents)
      DirEvents:=Struct("HANDLE[1000]")
    DirEvents[reset?reset:_.Length()]:=__[LP].O.hEvent
    DllCall("ReadDirectoryChangesW","UInt",__[LP].hD,"UInt",__[LP].FNI[""],"UInt",sizeof_FNI
           ,"Int",__[LP].sD,"UInt",__[LP].CNG,"UInt",0,"UInt",__[LP].O[""],"UInt",0)
    Return timer:=DllCall("SetTimer","Uint",0,"UInt",timer,"Uint",50,"UInt",WatchDirectory)
  } else {
    Sleep, 0
	If !DirEvents
		return
    for LP in reconnect
    {
      If (FileExist(__[LP].dir) && reconnect.Remove(LP)){
        DllCall("CloseHandle","Uint",__[LP].hD)
        __[LP].hD:=DllCall("CreateFile","Str",StrLen(__[LP].dir)=3?SubStr(__[LP].dir,1,2):__[LP].dir,"UInt",0x1,"UInt",0x1|0x2|0x4
                  ,"UInt",0,"UInt",0x3,"UInt",0x2000000|0x40000000,"UInt",0)
        DllCall("ResetEvent","UInt",__[LP].O.hEvent)
        DllCall("ReadDirectoryChangesW","UInt",__[LP].hD,"UInt",__[LP].FNI[""],"UInt",sizeof_FNI
               ,"Int",__[LP].sD,"UInt",__[LP].CNG,"UInt",0,"UInt",__[LP].O[""],"UInt",0)
      }
    }
    if !( (r:=DllCall("MsgWaitForMultipleObjectsEx","UInt",_.Length()
             ,"UInt",DirEvents[""],"UInt",0,"UInt",0x4FF,"UInt",6))>=0
             && r<_.Length() ){
      return
    }
    DllCall("KillTimer", UInt,0, UInt,timer)
    LP:=_[r+1],DllCall("GetOverlappedResult","UInt",__[LP].hD,"UInt",__[LP].O[""],"UIntP",nReadLen,"Int",1)
    If (A_LastError=64){ ; ERROR_NETNAME_DELETED - The specified network name is no longer available.
      If !FileExist(__[LP].dir) ; If folder does not exist add to reconnect routine
        reconnect[LP]:=LP
    } else
      Loop {
        FNI:=A_Index>1?(Struct(FILE_NOTIFY_INFORMATION,FNI[""]+FNI.NextEntryOffset)):(Struct(FILE_NOTIFY_INFORMATION,__[LP].FNI[""]))
        If (FNI.Action < 0x6){
          FileName:=__[LP].dir . StrGet(FNI.FileName[""],FNI.FileNameLength/2,"UTF-16")
          If (FNI.Action=FILE_ACTION_RENAMED_OLD_NAME)
            FileFromOptional:=FileName
          If (__[LP].FLT="" || RegExMatch(FileName,__[LP].FLT) || FileFrom)
            If (FNI.Action=FILE_ACTION_ADDED){
              FileTo:=FileName
            } else If (FNI.Action=FILE_ACTION_REMOVED){
              FileFrom:=FileName
            } else If (FNI.Action=FILE_ACTION_MODIFIED){
              FileFrom:=FileTo:=FileName
            } else If (FNI.Action=FILE_ACTION_RENAMED_OLD_NAME){
              FileFrom:=FileName
            } else If (FNI.Action=FILE_ACTION_RENAMED_NEW_NAME){
              FileTo:=FileName
            }
            If (FNI.Action != 4 && (FileTo . FileFrom) !="")
              __[LP].Func(FileFrom=""?FileFromOptional:FileFrom,FileTo)
              ,FileFrom:="",FileTo:="",FileFromOptional:=""
        }
      } Until (!FNI.NextEntryOffset || ((FNI[""]+FNI.NextEntryOffset) > (__[LP].FNI[""]+sizeof_FNI-12)))
    DllCall("ResetEvent","UInt",__[LP].O.hEvent)
    DllCall("ReadDirectoryChangesW","UInt",__[LP].hD,"UInt",__[LP].FNI[""],"UInt",sizeof_FNI
           ,"Int",__[LP].sD,"UInt",__[LP].CNG,"UInt",0,"UInt",__[LP].O[""],"UInt",0)
    timer:=DllCall("SetTimer","Uint",0,"UInt",timer,"Uint",50,"UInt",WatchDirectory)
    Return
  }
  Return
}