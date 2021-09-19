// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

/*
#ifdef _USRDLL
EXPORT int ahkPause(LPTSTR aChangeTo);
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkgetvar(LPTSTR name,unsigned int getVar = 0);
EXPORT int ahkassign(LPTSTR name, LPTSTR value);
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait);
EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait = 0);
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname) ;
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));

EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute = 0);
EXPORT int ahkExec(LPTSTR script);
#else
*/
EXPORT unsigned int NewThread(LPCTSTR aScript, LPCTSTR aCmdLine = _T(""), LPCTSTR aTitle = _T("AutoHotkey"));
EXPORT int ahkPause(LPTSTR aChangeTo, DWORD aThreadID = 0);
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName, DWORD aThreadID = 0);
EXPORT LPTSTR ahkgetvar(LPTSTR name, unsigned int getVar = 0, DWORD aThreadID = 0);
EXPORT int ahkassign(LPTSTR name, LPTSTR value, DWORD aThreadID = 0);
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line, unsigned int aMode, unsigned int wait, DWORD aThreadID = 0);
EXPORT int ahkLabel(LPTSTR aLabelName, unsigned int nowait = 0, DWORD aThreadID = 0);
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname, DWORD aThreadID = 0);
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""), DWORD aThreadID = 0);
EXPORT int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""), DWORD aThreadID = 0);

EXPORT int ahkReady(DWORD aThreadID = 0);
EXPORT UINT_PTR addScript(LPTSTR script, int waitexecute = 0, DWORD aThreadID = 0);
EXPORT int ahkExec(LPTSTR script, DWORD aThreadID = 0);
//#endif

void callFuncDll(FuncAndToken *aFuncAndToken); 
#ifndef _USRDLL
void callFuncDllVariant(FuncAndToken *aFuncAndToken); 
//EXPORT HANDLE ahkdll(LPTSTR fileName,LPTSTR argv, LPTSTR title);
//EXPORT HANDLE ahktextdll(LPTSTR fileName, LPTSTR argv, LPTSTR title);
//EXPORT int ahkReady(DWORD aThreadID);
//EXPORT HANDLE ahkReload(int timeout = 0, DWORD aThreadID = 0);
//EXPORT int ahkTerminate(int timeout = 0, DWORD aThreadID = 0);
EXPORT int com_ahkReady();
//EXPORT int com_ahkTerminate(int timeout, DWORD aThreadID = 0);
//void reloadDll();
//ResultType terminateDll(int aExitReason);


//COM virtual functions declaration
int com_ahkPause(LPTSTR aChangeTo, DWORD aThreadID);
UINT_PTR com_ahkFindLabel(LPTSTR aLabelName, DWORD aThreadID);
// LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar);
// unsigned int com_ahkassign(LPTSTR name, LPTSTR value);
UINT_PTR com_ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait, DWORD aThreadID);
int com_ahkLabel(LPTSTR aLabelName, unsigned int nowait, DWORD aThreadID);
UINT_PTR com_ahkFindFunc(LPTSTR funcname, DWORD aThreadID);
UINT_PTR com_addScript(LPTSTR script, int aExecute, DWORD aThreadID);
int com_ahkExec(LPTSTR script, DWORD aThreadID);

//HANDLE com_ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR title);
unsigned int com_newthread(LPTSTR fileName, LPTSTR argv, LPTSTR title);
//int com_ahkTerminate(int timeout, DWORD aThreadID);
int com_ahkReady(DWORD aThreadID);
//HANDLE com_ahkReload(int timeout, DWORD aThreadID);
#endif
#endif