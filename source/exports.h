
// Naveen v1. #define EXPORT __declspec(dllexport) 
#ifndef exports_h
#define exports_h

#define EXPORT extern "C" __declspec(dllexport)
#define BIF(fun) void fun(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount)

EXPORT unsigned int addFile(char *fileName, bool aAllowDuplicateInclude, int aIgnoreLoadFailure);
EXPORT unsigned int addScript(char *script, int aReplace);
EXPORT unsigned int ahkdll(char *fileName, char *argv, char *args);
EXPORT int ahkLabel(char *aLabelName);
EXPORT void* ahkFunction(char *func, char *param1, char *param2, char *param3, char *param4, char *param5, char *param6, char *param7, char *param8, char *param9, char *param10);
bool callFunc(WPARAM awParam, LPARAM alParam); 
// do not export callFunc, it must be called within script thread
void BIF_Import(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount);
void BIF_FindFunc(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount);
void BIF_Getvar(ExprTokenType &aResultToken, ExprTokenType *aParam[], int aParamCount);
BIF(BIF_Static) ;
BIF(BIF_Alias) ;
BIF(BIF_CacheEnable) ;
BIF(BIF_GetTokenValue) ;
int initPlugins();

EXPORT int ahkTerminate();
/*  ahkdll v10: disabling these as they aren't working
EXPORT int ahkContinue();
*/
#endif
