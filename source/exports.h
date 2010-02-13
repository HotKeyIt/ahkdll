// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)
#define BIF(fun) void fun(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)
#ifndef AUTOHOTKEYSC
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure);
EXPORT unsigned int addScript(LPTSTR script, int aReplace);
#endif
// EXPORT unsigned int ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR args);
EXPORT unsigned int ahkLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
EXPORT unsigned int ahkPostFunction(LPTSTR func, LPTSTR param1 = _T(""), LPTSTR param2 = _T(""), LPTSTR param3 = _T(""), LPTSTR param4 = _T(""), LPTSTR param5 = _T(""), LPTSTR param6 = _T(""), LPTSTR param7 = _T(""), LPTSTR param8 = _T(""), LPTSTR param9 = _T(""), LPTSTR param10 = _T(""));
bool callFunc(WPARAM awParam, LPARAM alParam); 
bool callFuncDll(); 
// do not export callFunc, it must be called within script thread
void BIF_FindFunc(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount);
void BIF_Getvar(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount);
BIF(BIF_Static) ;
BIF(BIF_Alias) ;
BIF(BIF_CacheEnable) ;
BIF(BIF_GetTokenValue) ;
int initPlugins();
#ifdef USRDLL
EXPORT int ahkReload();
void reloadDll();
ResultType terminateDll();
#endif

#endif
