// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)

EXPORT int ahkPause(LPTSTR aChangeTo);
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkgetvar(LPTSTR name,unsigned int getVar = 0);
EXPORT int ahkassign(LPTSTR name, LPTSTR value);
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait);
EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait = 0);
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname) ;
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));

#ifndef AUTOHOTKEYSC
EXPORT UINT_PTR addFile(LPTSTR fileName, int waitexecute = 0);
EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute = 0);
EXPORT int ahkExec(LPTSTR script);
#endif

void callFuncDllVariant(FuncAndToken *aFuncAndToken); 
void callFuncDll(FuncAndToken *aFuncAndToken); 

int initPlugins();

#ifdef _USRDLL
EXPORT UINT_PTR ahkdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
EXPORT UINT_PTR ahktextdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
EXPORT int ahkTerminate(DWORD timeout);
EXPORT int com_ahkTerminate(int timeout);
EXPORT int ahkReady();
EXPORT int com_ahkReady();
EXPORT int ahkReload(int timeout);
EXPORT int com_ahkReload();
void reloadDll();
ResultType terminateDll(int aExitReason);
EXPORT int ahkIsUnicode();
#endif

#ifndef MINIDLL
//COM virtual functions declaration
BOOL com_ahkPause(LPTSTR aChangeTo);
UINT_PTR com_ahkFindLabel(LPTSTR aLabelName);
// LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar);
// unsigned int com_ahkassign(LPTSTR name, LPTSTR value);
UINT_PTR com_ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait);
int com_ahkLabel(LPTSTR aLabelName, unsigned int nowait);
UINT_PTR com_ahkFindFunc(LPTSTR funcname);
// LPTSTR com_ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
unsigned int com_ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
#ifndef AUTOHOTKEYSC
UINT_PTR com_addScript(LPTSTR script, int aExecute);
int com_ahkExec(LPTSTR script);
UINT_PTR com_addFile(LPTSTR fileName, int waitexecute);
#endif
#ifdef _USRDLL
UINT_PTR com_ahkdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
UINT_PTR com_ahktextdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
int com_ahkTerminate(int timeout);
int com_ahkReady();
int com_ahkReload(int timeout);
int com_ahkIsUnicode();
#endif
#endif

#endif
// Additional exports of all exported functions
#ifndef _WIN64
#ifndef AUTOHOTKEYSC
#pragma comment(linker, "/export:ADDFILE=_addFile")
#pragma comment(linker, "/export:AddFile=_addFile")
#pragma comment(linker, "/export:Addfile=_addFile")
#pragma comment(linker, "/export:addfile=_addFile")
#pragma comment(linker, "/export:ADDSCRIPT=_addScript")
#pragma comment(linker, "/export:AddScript=_addScript")
#pragma comment(linker, "/export:Addscript=_addScript")
#pragma comment(linker, "/export:addscript=_addScript")
#endif
#pragma comment(linker, "/export:AHKASSIGN=_ahkassign")
#pragma comment(linker, "/export:AhkAssign=_ahkassign")
#pragma comment(linker, "/export:Ahkassign=_ahkassign")
#pragma comment(linker, "/export:ahkAssign=_ahkassign")
#ifdef _USRDLL
#pragma comment(linker, "/export:AHKDLL=_ahkdll")
#pragma comment(linker, "/export:AhkDll=_ahkdll")
#pragma comment(linker, "/export:ahkDll=_ahkdll")
#pragma comment(linker, "/export:Ahkdll=_ahkdll")
#endif
#ifndef AUTOHOTKEYSC
#pragma comment(linker, "/export:AHKEXEC=_ahkExec")
#pragma comment(linker, "/export:AhkExec=_ahkExec")
#pragma comment(linker, "/export:Ahkexec=_ahkExec")
#pragma comment(linker, "/export:ahkexec=_ahkExec")
#endif
#pragma comment(linker, "/export:AHKEXECUTELINE=_ahkExecuteLine")
#pragma comment(linker, "/export:AhkExecuteLine=_ahkExecuteLine")
#pragma comment(linker, "/export:AhkExecuteline=_ahkExecuteLine")
#pragma comment(linker, "/export:AhkexecuteLine=_ahkExecuteLine")
#pragma comment(linker, "/export:Ahkexecuteline=_ahkExecuteLine")
#pragma comment(linker, "/export:ahkExecuteline=_ahkExecuteLine")
#pragma comment(linker, "/export:ahkexecuteLine=_ahkExecuteLine")
#pragma comment(linker, "/export:ahkexecuteline=_ahkExecuteLine")
#pragma comment(linker, "/export:AHKFINDFUNC=_ahkFindFunc")
#pragma comment(linker, "/export:AhkFindFunc=_ahkFindFunc")
#pragma comment(linker, "/export:AhkFindfunc=_ahkFindFunc")
#pragma comment(linker, "/export:AhkfindFunc=_ahkFindFunc")
#pragma comment(linker, "/export:Ahkfindfunc=_ahkFindFunc")
#pragma comment(linker, "/export:ahkFindfunc=_ahkFindFunc")
#pragma comment(linker, "/export:ahkfindFunc=_ahkFindFunc")
#pragma comment(linker, "/export:ahkfindfunc=_ahkFindFunc")
#pragma comment(linker, "/export:AHKFINDLABEL=_ahkFindLabel")
#pragma comment(linker, "/export:AhkFindLabel=_ahkFindLabel")
#pragma comment(linker, "/export:AhkFindlabel=_ahkFindLabel")
#pragma comment(linker, "/export:AhkfindLabel=_ahkFindLabel")
#pragma comment(linker, "/export:Ahkfindlabel=_ahkFindLabel")
#pragma comment(linker, "/export:ahkFindlabel=_ahkFindLabel")
#pragma comment(linker, "/export:ahkfindLabel=_ahkFindLabel")
#pragma comment(linker, "/export:ahkfindlabel=_ahkFindLabel")
#pragma comment(linker, "/export:AHKFUNCTION=_ahkFunction")
#pragma comment(linker, "/export:AhkFunction=_ahkFunction")
#pragma comment(linker, "/export:Ahkfunction=_ahkFunction")
#pragma comment(linker, "/export:ahkfunction=_ahkFunction")
#pragma comment(linker, "/export:AHKGETVAR=_ahkgetvar")
#pragma comment(linker, "/export:AhkGetVar=_ahkgetvar")
#pragma comment(linker, "/export:AhkGetvar=_ahkgetvar")
#pragma comment(linker, "/export:AhkgetVar=_ahkgetvar")
#pragma comment(linker, "/export:Ahkgetvar=_ahkgetvar")
#pragma comment(linker, "/export:ahkGetVar=_ahkgetvar")
#pragma comment(linker, "/export:ahkGetvar=_ahkgetvar")
#pragma comment(linker, "/export:ahkgetVar=_ahkgetvar")
#pragma comment(linker, "/export:AHKISUNICODE=_ahkIsUnicode")
#pragma comment(linker, "/export:AhkIsUnicode=_ahkIsUnicode")
#pragma comment(linker, "/export:AhkIsunicode=_ahkIsUnicode")
#pragma comment(linker, "/export:AhkisUnicode=_ahkIsUnicode")
#pragma comment(linker, "/export:Ahkisunicode=_ahkIsUnicode")
#pragma comment(linker, "/export:ahkIsunicode=_ahkIsUnicode")
#pragma comment(linker, "/export:ahkisUnicode=_ahkIsUnicode")
#pragma comment(linker, "/export:ahkisunicode=_ahkIsUnicode")
#pragma comment(linker, "/export:AHKLABEL=_ahkLabel")
#pragma comment(linker, "/export:AhkLabel=_ahkLabel")
#pragma comment(linker, "/export:Ahklabel=_ahkLabel")
#pragma comment(linker, "/export:ahklabel=_ahkLabel")
#pragma comment(linker, "/export:AHKPAUSE=_ahkPause")
#pragma comment(linker, "/export:AhkPause=_ahkPause")
#pragma comment(linker, "/export:Ahkpause=_ahkPause")
#pragma comment(linker, "/export:ahkpause=_ahkPause")
#pragma comment(linker, "/export:AHKPOSTFUNCTION=_ahkPostFunction")
#pragma comment(linker, "/export:AhkPostFunction=_ahkPostFunction")
#pragma comment(linker, "/export:AhkPostfunction=_ahkPostFunction")
#pragma comment(linker, "/export:AhkpostFunction=_ahkPostFunction")
#pragma comment(linker, "/export:Ahkpostfunction=_ahkPostFunction")
#pragma comment(linker, "/export:ahkPostfunction=_ahkPostFunction")
#pragma comment(linker, "/export:ahkpostFunction=_ahkPostFunction")
#pragma comment(linker, "/export:ahkpostfunction=_ahkPostFunction")
#ifdef _USRDLL
#pragma comment(linker, "/export:AHKREADY=_ahkReady")
#pragma comment(linker, "/export:AhkReady=_ahkReady")
#pragma comment(linker, "/export:Ahkready=_ahkReady")
#pragma comment(linker, "/export:ahkready=_ahkReady")
#pragma comment(linker, "/export:AHKRELOAD=_ahkReload")
#pragma comment(linker, "/export:AhkReload=_ahkReload")
#pragma comment(linker, "/export:Ahkreload=_ahkReload")
#pragma comment(linker, "/export:ahkreload=_ahkReload")
#pragma comment(linker, "/export:AHKTERMINATE=_ahkTerminate")
#pragma comment(linker, "/export:AhkTerminate=_ahkTerminate")
#pragma comment(linker, "/export:Ahkterminate=_ahkTerminate")
#pragma comment(linker, "/export:ahkterminate=_ahkTerminate")
#pragma comment(linker, "/export:AHKTEXTDLL=_ahktextdll")
#pragma comment(linker, "/export:AhkTextDll=_ahktextdll")
#pragma comment(linker, "/export:AhkTextdll=_ahktextdll")
#pragma comment(linker, "/export:AhktextDll=_ahktextdll")
#pragma comment(linker, "/export:Ahktextdll=_ahktextdll")
#pragma comment(linker, "/export:ahkTextDll=_ahktextdll")
#pragma comment(linker, "/export:ahkTextdll=_ahktextdll")
#pragma comment(linker, "/export:ahktextDll=_ahktextdll")
#endif
#else // #ifndef _WIN64
#ifndef AUTOHOTKEYSC
#pragma comment(linker, "/export:ADDFILE=addFile")
#pragma comment(linker, "/export:AddFile=addFile")
#pragma comment(linker, "/export:Addfile=addFile")
#pragma comment(linker, "/export:addfile=addFile")
#pragma comment(linker, "/export:ADDSCRIPT=addScript")
#pragma comment(linker, "/export:AddScript=addScript")
#pragma comment(linker, "/export:Addscript=addScript")
#pragma comment(linker, "/export:addscript=addScript")
#endif
#pragma comment(linker, "/export:AHKASSIGN=ahkassign")
#pragma comment(linker, "/export:AhkAssign=ahkassign")
#pragma comment(linker, "/export:Ahkassign=ahkassign")
#pragma comment(linker, "/export:ahkAssign=ahkassign")
#ifdef _USRDLL
#pragma comment(linker, "/export:AHKDLL=ahkdll")
#pragma comment(linker, "/export:AhkDll=ahkdll")
#pragma comment(linker, "/export:ahkDll=ahkdll")
#pragma comment(linker, "/export:Ahkdll=ahkdll")
#endif
#ifndef AUTOHOTKEYSC
#pragma comment(linker, "/export:AHKEXEC=ahkExec")
#pragma comment(linker, "/export:AhkExec=ahkExec")
#pragma comment(linker, "/export:Ahkexec=ahkExec")
#pragma comment(linker, "/export:ahkexec=ahkExec")
#endif
#pragma comment(linker, "/export:AHKEXECUTELINE=ahkExecuteLine")
#pragma comment(linker, "/export:AhkExecuteLine=ahkExecuteLine")
#pragma comment(linker, "/export:AhkExecuteline=ahkExecuteLine")
#pragma comment(linker, "/export:AhkexecuteLine=ahkExecuteLine")
#pragma comment(linker, "/export:Ahkexecuteline=ahkExecuteLine")
#pragma comment(linker, "/export:ahkExecuteline=ahkExecuteLine")
#pragma comment(linker, "/export:ahkexecuteLine=ahkExecuteLine")
#pragma comment(linker, "/export:ahkexecuteline=ahkExecuteLine")
#pragma comment(linker, "/export:AHKFINDFUNC=ahkFindFunc")
#pragma comment(linker, "/export:AhkFindFunc=ahkFindFunc")
#pragma comment(linker, "/export:AhkFindfunc=ahkFindFunc")
#pragma comment(linker, "/export:AhkfindFunc=ahkFindFunc")
#pragma comment(linker, "/export:Ahkfindfunc=ahkFindFunc")
#pragma comment(linker, "/export:ahkFindfunc=ahkFindFunc")
#pragma comment(linker, "/export:ahkfindFunc=ahkFindFunc")
#pragma comment(linker, "/export:ahkfindfunc=ahkFindFunc")
#pragma comment(linker, "/export:AHKFINDLABEL=ahkFindLabel")
#pragma comment(linker, "/export:AhkFindLabel=ahkFindLabel")
#pragma comment(linker, "/export:AhkFindlabel=ahkFindLabel")
#pragma comment(linker, "/export:AhkfindLabel=ahkFindLabel")
#pragma comment(linker, "/export:Ahkfindlabel=ahkFindLabel")
#pragma comment(linker, "/export:ahkFindlabel=ahkFindLabel")
#pragma comment(linker, "/export:ahkfindLabel=ahkFindLabel")
#pragma comment(linker, "/export:ahkfindlabel=ahkFindLabel")
#pragma comment(linker, "/export:AHKFUNCTION=ahkFunction")
#pragma comment(linker, "/export:AhkFunction=ahkFunction")
#pragma comment(linker, "/export:Ahkfunction=ahkFunction")
#pragma comment(linker, "/export:ahkfunction=ahkFunction")
#pragma comment(linker, "/export:AHKGETVAR=ahkgetvar")
#pragma comment(linker, "/export:AhkGetVar=ahkgetvar")
#pragma comment(linker, "/export:AhkGetvar=ahkgetvar")
#pragma comment(linker, "/export:AhkgetVar=ahkgetvar")
#pragma comment(linker, "/export:Ahkgetvar=ahkgetvar")
#pragma comment(linker, "/export:ahkGetVar=ahkgetvar")
#pragma comment(linker, "/export:ahkGetvar=ahkgetvar")
#pragma comment(linker, "/export:ahkgetVar=ahkgetvar")
#pragma comment(linker, "/export:AHKISUNICODE=ahkIsUnicode")
#pragma comment(linker, "/export:AhkIsUnicode=ahkIsUnicode")
#pragma comment(linker, "/export:AhkIsunicode=ahkIsUnicode")
#pragma comment(linker, "/export:AhkisUnicode=ahkIsUnicode")
#pragma comment(linker, "/export:Ahkisunicode=ahkIsUnicode")
#pragma comment(linker, "/export:ahkIsunicode=ahkIsUnicode")
#pragma comment(linker, "/export:ahkisUnicode=ahkIsUnicode")
#pragma comment(linker, "/export:ahkisunicode=ahkIsUnicode")
#pragma comment(linker, "/export:AHKLABEL=ahkLabel")
#pragma comment(linker, "/export:AhkLabel=ahkLabel")
#pragma comment(linker, "/export:Ahklabel=ahkLabel")
#pragma comment(linker, "/export:ahklabel=ahkLabel")
#pragma comment(linker, "/export:AHKPAUSE=ahkPause")
#pragma comment(linker, "/export:AhkPause=ahkPause")
#pragma comment(linker, "/export:Ahkpause=ahkPause")
#pragma comment(linker, "/export:ahkpause=ahkPause")
#pragma comment(linker, "/export:AHKPOSTFUNCTION=ahkPostFunction")
#pragma comment(linker, "/export:AhkPostFunction=ahkPostFunction")
#pragma comment(linker, "/export:AhkPostfunction=ahkPostFunction")
#pragma comment(linker, "/export:AhkpostFunction=ahkPostFunction")
#pragma comment(linker, "/export:Ahkpostfunction=ahkPostFunction")
#pragma comment(linker, "/export:ahkPostfunction=ahkPostFunction")
#pragma comment(linker, "/export:ahkpostFunction=ahkPostFunction")
#pragma comment(linker, "/export:ahkpostfunction=ahkPostFunction")
#ifdef _USRDLL
#pragma comment(linker, "/export:AHKREADY=ahkReady")
#pragma comment(linker, "/export:AhkReady=ahkReady")
#pragma comment(linker, "/export:Ahkready=ahkReady")
#pragma comment(linker, "/export:ahkready=ahkReady")
#pragma comment(linker, "/export:AHKRELOAD=ahkReload")
#pragma comment(linker, "/export:AhkReload=ahkReload")
#pragma comment(linker, "/export:Ahkreload=ahkReload")
#pragma comment(linker, "/export:ahkreload=ahkReload")
#pragma comment(linker, "/export:AHKTERMINATE=ahkTerminate")
#pragma comment(linker, "/export:AhkTerminate=ahkTerminate")
#pragma comment(linker, "/export:Ahkterminate=ahkTerminate")
#pragma comment(linker, "/export:ahkterminate=ahkTerminate")
#pragma comment(linker, "/export:AHKTEXTDLL=ahktextdll")
#pragma comment(linker, "/export:AhkTextDll=ahktextdll")
#pragma comment(linker, "/export:AhkTextdll=ahktextdll")
#pragma comment(linker, "/export:AhktextDll=ahktextdll")
#pragma comment(linker, "/export:Ahktextdll=ahktextdll")
#pragma comment(linker, "/export:ahkTextDll=ahktextdll")
#pragma comment(linker, "/export:ahkTextdll=ahktextdll")
#pragma comment(linker, "/export:ahktextDll=ahktextdll")
#endif
#endif // _WIN64