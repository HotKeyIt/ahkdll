// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)
#define BIF(fun) void fun(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)
#ifndef AUTOHOTKEYSC
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude = false, int aIgnoreLoadFailure = 0);
EXPORT unsigned int addScript(LPTSTR script, int aReplace = 0);
#endif
// EXPORT unsigned int ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR args);
EXPORT unsigned int ahkLabel(LPTSTR aLabelName, unsigned int wait = 0);
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT unsigned int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
bool callFunc(WPARAM awParam, LPARAM alParam); 
bool callFuncDll(FuncAndToken *aFuncAndToken); 
// do not export callFunc, it must be called within script thread
BIF(BIF_FindFunc);
BIF(BIF_Getvar);
BIF(BIF_Static) ;
BIF(BIF_Alias) ;
BIF(BIF_CacheEnable) ;
BIF(BIF_getTokenValue) ;
int initPlugins();
EXPORT unsigned int ahkFindFunc(LPTSTR funcname) ;
#ifdef USRDLL
EXPORT int ahkReload();
void reloadDll();
ResultType terminateDll();
#endif

#endif
