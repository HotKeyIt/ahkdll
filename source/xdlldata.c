// Wrapper für "dlldata.c"

#ifdef _MERGE_PROXYSTUB // Proxy-Stub-DLL zusammenführen

#define REGISTER_PROXY_DLL //DllRegisterServer usw.

#define USE_STUBLESS_PROXY	//nur mit dem MIDL-Schalter "/Oicf" definiert

#pragma comment(lib, "rpcns4.lib")
#pragma comment(lib, "rpcrt4.lib")

#define ENTRY_PREFIX	Prx

#include "dlldata.c"
#include "AutoHotkey2_p.c"

#endif //_MERGE_PROXYSTUB
