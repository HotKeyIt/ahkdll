
#pragma once
#ifdef _USRDLL
//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       const CLSID& clsid, 
                       const char* szFriendlyName,
                       const char* szVerIndProgID,
                       const char* szProgID,
					   const CLSID& libid) ;

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(const CLSID& clsid,
                         const char* szVerIndProgID,
                         const char* szProgID,
						 const CLSID& libid) ;


void RegisterInterface(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       const char* szFriendlyName, // Friendly Name
					   const CLSID &libid,
					   const IID &iid);
void UnregisterInterface(const IID &iid);


HRESULT RegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
HRESULT UnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);

#endif