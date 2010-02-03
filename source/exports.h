// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)
#define BIF(fun) void fun(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)
EXPORT unsigned int addFile(LPTSTR fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure);
EXPORT unsigned int addScript(LPTSTR script, int aReplace);
// EXPORT unsigned int ahkdll(LPTSTR fileName, LPTSTR argv, LPTSTR args);
EXPORT unsigned int ahkLabel(LPTSTR aLabelName);
EXPORT LPTSTR ahkFunction(LPTSTR func, LPTSTR param1, LPTSTR param2, LPTSTR param3, LPTSTR param4, LPTSTR param5, LPTSTR param6, LPTSTR param7, LPTSTR param8, LPTSTR param9, LPTSTR param10);
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

// EXPORT int ahkTerminate();
EXPORT int ahkReload();
EXPORT int ahkPause(LPTSTR aChangeTo);
void reloadDll();
ResultType terminateDll();
/*  ahkdll v10: disabling these as they aren't working
EXPORT int ahkContinue();
*/
#endif
