// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)

EXPORT BOOL ahkPause(LPTSTR aChangeTo);
EXPORT UINT_PTR ahkFindLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkgetvar(LPTSTR name,unsigned int getVar = 0);
EXPORT unsigned int ahkassign(LPTSTR name, LPTSTR value);
EXPORT UINT_PTR ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait);
EXPORT BOOL ahkLabel(LPTSTR aLabelName, unsigned int nowait = 0);
EXPORT UINT_PTR ahkFindFunc(LPTSTR funcname) ;
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT unsigned int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));

#ifndef AUTOHOTKEYSC
EXPORT UINT_PTR addFile(LPTSTR fileName, bool aAllowDuplicateInclude = false, int aIgnoreLoadFailure = 0);
EXPORT UINT_PTR addScript(LPTSTR script, int aReplace = 0);
EXPORT BOOL ahkExec(LPTSTR script);
#endif

void callFuncDllVariant(FuncAndToken *aFuncAndToken); 
void callFuncDll(FuncAndToken *aFuncAndToken); 

int initPlugins();

#ifdef _USRDLL
EXPORT UINT_PTR ahkdll(LPTSTR fileName,LPTSTR argv);
EXPORT UINT_PTR ahktextdll(LPTSTR fileName,LPTSTR argv);
EXPORT BOOL ahkTerminate(int timeout);
EXPORT BOOL com_ahkTerminate(int timeout);
EXPORT BOOL ahkReady();
EXPORT BOOL com_ahkReady();
EXPORT BOOL ahkReload(int timeout);
EXPORT BOOL com_ahkReload();
void reloadDll();
ResultType terminateDll();
#endif

#endif

#ifndef MINIDLL
//COM virtual functions declaration
BOOL com_ahkPause(LPTSTR aChangeTo);
UINT_PTR com_ahkFindLabel(LPTSTR aLabelName);
// LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar);
// unsigned int com_ahkassign(LPTSTR name, LPTSTR value);
UINT_PTR com_ahkExecuteLine(UINT_PTR line,unsigned int aMode,unsigned int wait);
BOOL com_ahkLabel(LPTSTR aLabelName, unsigned int nowait);
UINT_PTR com_ahkFindFunc(LPTSTR funcname);
// LPTSTR com_ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
unsigned int com_ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
#ifndef AUTOHOTKEYSC
UINT_PTR com_addScript(LPTSTR script, int aExecute);
BOOL com_ahkExec(LPTSTR script);
UINT_PTR com_addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure);
#endif
#ifdef _USRDLL
UINT_PTR com_ahkdll(LPTSTR fileName,LPTSTR argv);
UINT_PTR com_ahktextdll(LPTSTR fileName,LPTSTR argv);
BOOL com_ahkTerminate(int timeout);
BOOL com_ahkReady();
BOOL com_ahkReload(int timeout);
#endif
#endif