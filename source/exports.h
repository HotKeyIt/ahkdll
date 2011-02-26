// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)
#define BIF(fun) void fun(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)

EXPORT BOOL ahkPause(LPTSTR aChangeTo);
EXPORT unsigned int ahkFindLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkgetvar(LPTSTR name,unsigned int getVar);
EXPORT unsigned int ahkassign(LPTSTR name, LPTSTR value);
EXPORT unsigned int ahkExecuteLine(unsigned int line,unsigned int aMode,unsigned int wait);
EXPORT BOOL ahkLabel(LPTSTR aLabelName, unsigned int nowait = 0);
EXPORT unsigned int ahkFindFunc(LPTSTR funcname) ;
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT unsigned int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT BOOL ahkKey(LPTSTR keys);

#ifndef AUTOHOTKEYSC
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude = false, int aIgnoreLoadFailure = 0);
EXPORT unsigned int addScript(LPTSTR script, int aReplace = 0);
EXPORT BOOL ahkExec(LPTSTR script);
#endif

void callFuncDllVariant(FuncAndToken *aFuncAndToken); 
void callFuncDll(FuncAndToken *aFuncAndToken); 

BIF(BIF_FindFunc);
BIF(BIF_Getvar);
BIF(BIF_Static) ;
BIF(BIF_Alias) ;
BIF(BIF_CacheEnable) ;
BIF(BIF_getTokenValue) ;
int initPlugins();

#ifdef _USRDLL
EXPORT unsigned int ahkdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
EXPORT unsigned int ahktextdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
EXPORT BOOL ahkTerminate(bool kill=0);
EXPORT BOOL com_ahkTerminate(bool kill);
EXPORT BOOL ahkReady();
EXPORT BOOL com_ahkReady();
EXPORT BOOL ahkReload();
EXPORT BOOL com_ahkReload();
void reloadDll();
ResultType terminateDll();
#endif

#endif

#ifndef MINIDLL
//COM virtual functions declaration
BOOL com_ahkPause(LPTSTR aChangeTo);
unsigned int com_ahkFindLabel(LPTSTR aLabelName);
// LPTSTR com_ahkgetvar(LPTSTR name,unsigned int getVar);
// unsigned int com_ahkassign(LPTSTR name, LPTSTR value);
unsigned int com_ahkExecuteLine(unsigned int line,unsigned int aMode,unsigned int wait);
BOOL com_ahkLabel(LPTSTR aLabelName, unsigned int nowait);
unsigned int com_ahkFindFunc(LPTSTR funcname);
// LPTSTR com_ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
unsigned int com_ahkPostFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
BOOL com_ahkKey(LPTSTR keys);
#ifndef AUTOHOTKEYSC
unsigned int com_addScript(LPTSTR script, int aExecute);
BOOL com_ahkExec(LPTSTR script);
unsigned int com_addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure);
#endif
#ifdef _USRDLL
unsigned int com_ahkdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
unsigned int com_ahktextdll(LPTSTR fileName,LPTSTR argv,LPTSTR args);
BOOL com_ahkTerminate(bool kill);
BOOL com_ahkReady();
BOOL com_ahkReload();
#endif
#endif