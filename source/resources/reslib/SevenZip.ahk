SevenZip(hwnd:=0){
  static FILETIME := "dwLowDateTime,dwHighDateTime"
        ,INDIVIDUALINFO := "1:DWORD dwOriginalSize;DWORD dwCompressedSize;DWORD dwCRC;UINT uFlag;UINT uOSType;WORD wRatio;WORD wDate;WORD wTime;char szFileName[513];char dummy1[3];char szAttribute[8];char szMode[8]"
        ,EXTRACTINGINFO:="1:DWORD dwFileSize;DWORD dwWriteSize;char szSourceFileName[513];char dummy1[3];char szDestFileName[513];char dummy[3]"
        ,EXTRACTINGINFOEX := "1:SevenZip(EXTRACTINGINFO) exinfo;DWORD dwCompressedSize;DWORD dwCRC;UINT uOSType;WORD wRatio;WORD wDate;WORD wTime;char szAttribute[8];char szMode[8]"
        ,EXTRACTINGINFOEX64:="1:DWORD dwStructSize;SevenZip(EXTRACTINGINFO) exinfo;INT64 llFileSize;INT64 llCompressedSize;INT64 llWriteSize;DWORD dwAttributes;DWORD dwCRC;UINT uOSType;WORD wRatio;SevenZip(FILETIME) ftCreateTime;SevenZip(FILETIME) ftAccessTime;SevenZip(FILETIME) ftWriteTime;char szMode[8];char szSourceFileName[513];char dummy1[3];char szDestFileName[513];char dummy2[3]"
        ,EXTRACTINGINFOEX32:="1:DWORD dwStructSize;SevenZip(EXTRACTINGINFO) exinfo;DWORD dwFileSize;DWORD dwCompressedSize;DWORD dwWriteSize;DWORD dwAttributes;DWORD dwCRC;UINT uOSType;WORD wRatio;SevenZip(FILETIME) ftCreateTime;SevenZip(FILETIME) ftAccessTime;SevenZip(FILETIME) ftWriteTime;char szMode[8];char szSourceFileName[513];char dummy1[3];char szDestFileName[513];char dummy2[3]"
  return new SevenZip(hwnd)
}
Class SevenZip{
  static INDIVIDUALINFO := "1:DWORD dwOriginalSize;DWORD dwCompressedSize;DWORD dwCRC;UINT uFlag;UINT uOSType;WORD wRatio;WORD wDate;WORD wTime;char szFileName[513];char dummy1[3];char szAttribute[8];char szMode[8]"
  static Error:=["ERROR_WARNING: Something the error which is not fatal occurred.","ERROR_FATAL: Something fatal error occurred.","ERROR_FILE_CRC: Checksum of the housing file is not agreeable.","ERROR_READ_ONLY: The file is read-only.","ERROR_CANNOT_WRITE: Write-error occurred.","ERROR_FILE_OPEN: The file was not opened."
              ,"ERROR_COMMAND_NAME: Command appointment by mistake is.","ERROR_ENOUGH_MEMORY: Global memory is insufficient.","ERROR_NOT_ARC_FILE: The file which it appoints is not the archive file.","ERROR_VERSION: The housing file it is type of the version to which 7-ZIP32.DLL is not corresponding."
              ,"ERROR_METHOD: It is housed with the housing mode which cannot be handled with 7-ZIP32.DLL.","ERROR_DURING_DECOMPRESSION: When thawing error occurred.","ERROR_DIR_FILE_WIDTH_64BIT_SIZE: The file	 of the directory is the size of 64bit.","ERROR_CONVERT_TIME: It cannot convert time stamp."
              ,"ERROR_FILE_CHANGED_DURING_OPERATION: The file was modified while operating.","ERROR_USER_CANCEL: Processing was discontinued by the user.","ERROR_ALREADY_RUNNING: Already 7-ZIP is in the midst of operating."
              ,"ERROR_HARC_ISNOT_OPENED: SevenZipOpenArchive () with relation before attaching the book room and the steering wheel, SevenZipFindFirst () and so on API was used.","ERROR_NOT_SEARCH_MODE: SevenZipFindFirst () before using, SevenZipFindNext () was called and high before calling these, SevenZipGetFileName () and so on API was called."
              ,"ERROR_PASSWORD_FILE: The password which is input by mistake is."]
  __New(hwnd:=0){
    sz:=UnZipRawMemory(LockResource(LoadResource(0,hRes:=DllCall("FindResource","PTR",0,"Str","556EA2A65AE54D58BC52C792B3ED2ED0","PTR",10,"PTR"))),SizeofResource(0,hRes),dll)
    this.lib:=lib:=MemoryLoadLibrary(&dll,sz)
    for k,v in {Zip:"ttti",GetVersion:"h=s",GetCursorMode:"i",SetCursorMode:"i",GetBackGroundMode:"",SetBackGroundMode:"i",GetCursorInterval:"h=",SetCursorInterval:"h",GetRunning:"",ConfigDialog:"tti",_CheckArchive:"ti",_GetArchiveType:"t",_GetFileCount:"t",_OpenArchive:"t=tTi",CloseArchive:"t",_FindFirst:"ttt",FindNext:"tt",_GetArcFileName:"tti",GetArcFileSize:"ui=t",GetArcOriginalSize:"ui=t",GetArcCompressedSize:"ui=t",GetArcRatio:"h=t",GetArcDate:"h=t",GetArcTime:"h=t",GetArcOSType:"ui=t",IsSFXFile:"t",_GetFileName:"tti",GetOriginalSize:"ui=t",GetCompressedSize:"ui=t",GetRatio:"h=t",GetDate:"h=t",GetTime:"h=t",GetCRC:"ui=t",GetAttribute:"t",GetOSType:"ui=t",_GetMethod:"tti",GetWriteTime:"ui=t",GetWriteTimeEx:"tt",GetArcCreateTimeEx:"tt",GetArcAccessTimeEx:"tt",GetArcWriteTimeEx:"tt",SetOwnerWindow:"t",ClearOwnerWindow:"",SetOwnerWindowEx:"tt",KillOwnerWindowEx:"t",SetOwnerWindowEx64:"tti",KillOwnerWindowEx64:"t",GetSubVersion:"h=",GetArcFileSizeEx:"tt",GetArcOriginalSizeEx:"tt",GetArcCompressedSizeEx:"tt",GetOriginalSizeEx:"tt",GetCompressedSizeEx:"tt",SetUnicodeMode:"i",_ExtractMem:"tttittt",_ExtractMemEx:"ttti6ttt",SetPriority:"i",SetCP:"ui",GetCP:"",GetLastError:"t",_SetDefaultPassword:"tt",_GetDefaultPassword:"tti",_PasswordDialog:"tti",_SfxFileStoring:"t",_SfxConfigDialog:"tti"}
      this[StrReplace(k,"Archive","")]:=DynaCall(MemoryGetProcAddress(lib,"SevenZip" (k="zip"?"":LTrim(k,"_"))),v)
    this.SetUnicodeMode(1), this.hwnd:=hwnd
  }
  ArrayToList(files){
    list:=""
    if !IsObject(files)
      return StrReplace(files,"`n",""" """)
    for k, v in files
      list.=v "`n"
    return StrReplace(RTrim(list,"`n"),"`n",""" """)
  }
  SetDefaultPassword(harc:=0, pw:=""){
    StrPutVar(pw,_pw,"UTF-8")
    return this._SetDefaultPassword(harc,&_pw)
  }
  GetDefaultPassword(harc:=0){
    if (-1=sz:=this._GetDefaultPassword(harc))
      return -1
    VarSetCapacity(pw,sz)
    this._GetDefaultPassword(harc,&pw,sz)
    return StrGet(&pw,"UTF-8")
  }
  PasswordDialog(){
    VarSetCapacity(pw,10240)
    if !this._PasswordDialog(this.hwnd,&pw,10240)
      return StrGet(&pw,"UTF-8")
  }
  SfxConfigDialog(){
    VarSetCapacity(sfx,10240)
    this._SfxConfigDialog(this.hwnd,&sfx,10240)
    return StrGet(&sfx,"UTF-8")
  }
  SfxFileStoring(file){
    StrPutVar(file,_file,"UTF-8")
    return this._SfxFileStoring(&_file)
  }
  AutoZip(files, method:="zip", level:=9, threads:=2, options:=""){
    if !IsObject(files)
      _files:=files
    else
      for k,v in files
        _files.=v "`n"
    Loop,Parse,_files,`n,`r
    {
      If FileExist(A_LoopField){
        If (this.Check(A_LoopField)){
          FileCreateDir % tempDir:=A_Temp "\" A_TickCount "_extract"
          this.Extract(A_LoopField,tempDir,, "-y" (InStr(options,"-hide")?" -hide":""))
          FileDelete,%A_LoopField%
          FileDelete,%tempDir%.txt
          Loop,%tempDir%\*,1
            FileAppend,%A_LoopFileFullPath%`n,%tempDir%.txt
          result:=this.Add(SubStr(A_LoopField,-1*StrLen(method))=method?A_LoopField:SubStr(A_LoopField,1,InStr(A_LoopField,".",1,-1)) method,"@" tempdir ".txt","-t" method " -r -mx" level " -mmt=" threads " " options)
          FileRemoveDir,%tempDir%,1
          FileDelete,%tempDir%.txt
        } else result:=this.Add(((isFolder:=InStr(FileExist(A_LoopField),"D"))?RTrim(A_LoopField,"\") ".":SubStr(A_LoopField,1,InStr(A_LoopField,".",1,-1))) method, A_LoopField (isFolder?"\*":""), "-t" method " " (isFolder?"-r ":"") "-mx" level " -mmt=" threads)
      }
    }
    return result
  }
  ExtractMem(ByRef buf, archive, file:="", options:=""){
    StrPutVar("x " options " """ archive """" (file ? " """ file """":""),command,"UTF-8")
  if (!harc:=this.Open(archive,0))
    || (!info:=this.FindFirst(harc,file))
    throw Exception("Could not open archive or archive is empty: " archive,-1)
  szBuf:=info.dwOriginalSize
  if (file="")
    for k,v in (szBuf:=0, this.List(archive))
      szBuf+=v[4]
  this.Close(harc)
  VarSetCapacity(buf,szBuf)
    VarSetCapacity(time,8,0), VarSetCapacity(attr,4,0), VarSetCapacity(szWrite,8,0)
    if Err:=this._ExtractMem(this.hwnd,&command,&buf,szBuf,&time,&attr,&szWrite)
      throw Exception(SevenZip.Error[Err],-1)
  else return szBuf
  }
  ExtractMemEx(ByRef buf, archive, file:="", options:=""){
    StrPutVar("x " options " """ archive """" (file ? " """ file """":""),command,"UTF-8")
  if (!harc:=this.Open(archive,0))
    || (!info:=this.FindFirst(harc,file))
    throw Exception("Could not open archive or archive is empty: " archive,-1)
  szBuf:=info.dwOriginalSize
  if (file="")
    for k,v in (szBuf:=0, this.List(archive))
      szBuf+=v[4]
  this.Close(harc)
  VarSetCapacity(buf,szBuf)
    VarSetCapacity(time,8,0), VarSetCapacity(attr,4,0), VarSetCapacity(szWrite,8,0)
    if Err:=this._ExtractMemEx(this.hwnd,&command,&buf,szBuf,&time,&attr,&szWrite)
      throw Exception(SevenZip.Error[Err],-1)
  else return szBuf
  }
  FindFirst(harc, file:=""){
    StrPutVar("""" this.ArrayToList(file) """",_file,"UTF-8")
    info:=Struct(SevenZip.INDIVIDUALINFO)
    if !this._FindFirst(harc, &_file, info[])
      return info
  }
  GetMethod(harc){
    VarSetCapacity(buf,10,0)
    if !Err:=this._GetMethod(harc, &buf, 8)
      return StrGet(&buf,"UTF-8")
    else return SevenZip.Error[Err]
  }
  GetFileName(harc){
    VarSetCapacity(buf,1024)
    if !Err:=this._GetFileName(harc, &buf, 1024)
      return StrGet(&buf,"UTF-8")
    else return SevenZip.Error[Err]
  }
  GetArcFileName(harc){
    VarSetCapacity(buf,102400)
    if !Err:=this._GetArcFileName(harc, &buf, 102400)
      return StrGet(&buf,"UTF-8")
    else return SevenZip.Error[Err]
  }
  Open(archive, mode){
    StrPutVar(archive,file,"UTF-8")
    return this._Open(this.hwnd, &file, mode)
  }
  Check(archive, mode:=0){
    StrPutVar(archive,file,"UTF-8")
    return this._Check(&file, mode)
  }
  GetFileCount(archive){
    StrPutVar(archive,file,"UTF-8")
    return this._GetFileCount(&file)
  }
  GetType(archive){
    StrPutVar(archive,file,"UTF-8")
    return this._GetType(&file)
  }
  Cmd(commandline){
    VarSetCapacity(buf,1024)
    StrPutVar(commandline,command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Add(archive, files, options:=""){
    VarSetCapacity(buf,10240)
    StrPutVar("a " options " """ archive """ """ this.ArrayToList(files) """",command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,10240)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Benchmark(options:=""){
    VarSetCapacity(buf,10240)
    StrPutVar("b " options,command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,10240)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
  else return StrGet(&buf,"UTF-8")
  }
  Delete(archive, files, options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("d " options " """ archive """ """ this.ArrayToList(files) """",command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  ExtractRoot(archive, dir, files:="", options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("e " options " """ archive """" (dir?" -o""" RTrim(dir,"\") "\""":"") (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  List(archive, files:="", options:=""){
    VarSetCapacity(buf,1024*1024)
    StrPutVar("l " options " """ archive """" (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024*1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    content:=false, out:=[], buffer:=StrGet(&buf,"UTF-8")
    Loop,Parse, buffer, `n, `r
      If content
        if (A_LoopField="------------------- ----- ------------ ------------  ------------------------")
          return out
        else out.Push([StrSplit(RegexReplace(A_LoopField,"([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)?\s+?(.*)","$1" A_Tab "$2" A_Tab "$3" A_Tab "$4" A_Tab "$5" A_Tab "$6"),A_Tab)*])
      else if (A_LoopField="------------------- ----- ------------ ------------  ------------------------")
        content:=true
  }
  Test(archive, files:="", options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("t " options " """ archive """" (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Update(archive, files, options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("u " options " """ archive """" (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Extract(archive, dir, files:="", options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("x " options " """ archive """" (dir?" -o""" RTrim(dir,"\") "\""":"") (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Hash(crc:="", files:="", options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("h -scrc" (crc=""?"CRC32":crc) " " options " " (files?"""" this.ArrayToList(files) """":"*"),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  Rename(archive, files, options:=""){
    VarSetCapacity(buf,1024)
    StrPutVar("rn " options " """ archive """" (files?" """ this.ArrayToList(files) """":""),command,"UTF-8")
    if this.Zip(this.hwnd,&command,&buf,1024)
      throw Exception(StrGet(&buf,"UTF-8"),-1)
    return StrGet(&buf,"UTF-8")
  }
  __Delete(){
    MemoryFreeLibrary(this.lib)
  }
}