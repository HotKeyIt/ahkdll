// COMServer.cpp: Implementierung von CCOMServer
#include "pch.h"
#ifndef _USRDLL
#include "COMServer.h"
#include "globaldata.h" // for access to many global vars
#include "application.h" // for MsgSleep()
#include "window.h" // For MsgBox()
#include "TextIO.h"
#include "LiteZip.h"
#include "Debugger.h"
#include "script_com.h"
#include <process.h>


// CCOMServer


//
// Constructor
//
CCOMServer::CCOMServer()
{
	m_pUnkMarshaler = nullptr;
	//InterlockedIncrement(&g_cComponents);

	//m_ptinfo = NULL;
	//LoadTypeInfo(&m_ptinfo, LIBID_AutoHotkey2, IID_ICOMServer, 0);
}








STDMETHODIMP CCOMServer::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_ICOMServer
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}







//
// AutoHotkey2COMServer functions
//

LPTSTR Variant2T(VARIANT var, LPTSTR buf)
{
	USES_CONVERSION;
	if (var.vt == VT_BYREF + VT_VARIANT)
		var = *var.pvarVal;
	if (var.vt == VT_ERROR)
		return _T("");
	else if (var.vt == VT_BSTR)
		return OLE2T(var.bstrVal);
	else if (var.vt == VT_I2 || var.vt == VT_I4 || var.vt == VT_I8)
#ifdef _WIN64
		return _ui64tot(var.uintVal, buf, 10);
#else
		return _ultot(var.uintVal, buf, 10);
#endif
	return _T("");
}

unsigned int Variant2I(VARIANT var)
{
	USES_CONVERSION;
	if (var.vt == VT_BYREF + VT_VARIANT)
		var = *var.pvarVal;
	if (var.vt == VT_ERROR)
		return 0;
	else if (var.vt == VT_BSTR)
		return ATOI(OLE2T(var.bstrVal));
	else //if (var.vt==VT_I2 || var.vt==VT_I4 || var.vt==VT_I8)
		return var.uintVal;
}

HRESULT __stdcall CCOMServer::NewThread(/*in,optional*/VARIANT script,/*in,optional*/VARIANT params,/*in,optional*/VARIANT title,/*out*/unsigned int* ThreadID)
{
	USES_CONVERSION;
	TCHAR buf1[MAX_INTEGER_SIZE], buf2[MAX_INTEGER_SIZE], buf3[MAX_INTEGER_SIZE];
	if (ThreadID == NULL)
		return ERROR_INVALID_PARAMETER;
	*ThreadID = (unsigned int)com_newthread(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script, buf1)
		, params.vt == VT_BSTR ? OLE2T(params.bstrVal) : Variant2T(params, buf2)
		, title.vt == VT_BSTR ? OLE2T(title.bstrVal) : Variant2T(title, buf3));
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkPause(/*in*/ VARIANT aThreadID,/*in,optional*/VARIANT aChangeTo,/*out*/int* paused)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (paused == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*paused = com_ahkPause(aChangeTo.vt == VT_BSTR ? OLE2T(aChangeTo.bstrVal) : Variant2T(aChangeTo, buf), ThreadID);
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkReady(/*in*/ VARIANT aThreadID,/*out*/int* ready)
{
	DWORD ThreadID = Variant2I(aThreadID);
	if (ready == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	*ready = com_ahkReady(ThreadID);
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkFindLabel(/*in*/ VARIANT aThreadID,/*in*/VARIANT aLabelName,/*out*/UINT_PTR* aLabelPointer)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (aLabelPointer == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*aLabelPointer = com_ahkFindLabel(aLabelName.vt == VT_BSTR ? OLE2T(aLabelName.bstrVal) : Variant2T(aLabelName, buf), ThreadID);
	return S_OK;
}

HRESULT __stdcall CCOMServer::ahkgetvar(/*in*/ VARIANT aThreadID,/*in*/VARIANT name,/*[in,optional]*/ VARIANT getVar,/*out*/VARIANT* result)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (result == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	
	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	for (int i = 0; i < MAX_AHK_THREADS; i++)
	{
		if (g_ahkThreads[i][5] == ThreadID)
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
			break;
		}
	}
	if (!curr_teb)
		return E_FAIL;

	TCHAR buf[MAX_INTEGER_SIZE];
	Var* var;
	ExprTokenType aToken;

	var = g_script->FindGlobalVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name, buf));
	var->ToToken(aToken);
	VariantInit(result);
	
	VARIANT b;
	TokenToVariant(aToken, b, FALSE);
	HRESULT aRes = VariantCopy(result, &b);
	curr_teb->ThreadLocalStoragePointer = tls;
	return aRes;
}

void AssignVariant(Var& aArg, VARIANT& aVar, bool aRetainVar = true);

HRESULT __stdcall CCOMServer::ahkassign(/*in*/ VARIANT aThreadID,/*in*/VARIANT name, /*in*/VARIANT value,/*out*/int* success)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (success == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;

	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	for (int i = 0; i < MAX_AHK_THREADS; i++)
	{
		if (g_ahkThreads[i][5] == ThreadID)
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
			break;
		}
	}
	if (!curr_teb)
		return E_FAIL;

	TCHAR namebuf[MAX_INTEGER_SIZE];
	Var* var;
	if (!(var = g_script->FindGlobalVar(name.vt == VT_BSTR ? OLE2T(name.bstrVal) : Variant2T(name, namebuf))))
	{
		curr_teb->ThreadLocalStoragePointer = tls;
		return ERROR_INVALID_PARAMETER;  // Realistically should never happen.
	}
	AssignVariant(*var, value, false);
	curr_teb->ThreadLocalStoragePointer = tls;
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkExecuteLine(/*in*/ VARIANT aThreadID,/*[in,optional]*/ VARIANT line,/*[in,optional]*/ VARIANT aMode,/*[in,optional]*/ VARIANT wait,/*[out, retval]*/ UINT_PTR* pLine)
{
	DWORD ThreadID = Variant2I(aThreadID);
	if (pLine == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	*pLine = com_ahkExecuteLine(Variant2I(line), Variant2I(aMode), Variant2I(wait), ThreadID);
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkLabel(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ int* success)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (success == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_ahkLabel(aLabelName.vt == VT_BSTR ? OLE2T(aLabelName.bstrVal) : Variant2T(aLabelName, buf)
		, Variant2I(nowait), ThreadID);
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkFindFunc(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT FuncName,/*[out, retval]*/ UINT_PTR* pFunc)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (pFunc == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*pFunc = com_ahkFindFunc(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName, buf), ThreadID);
	return S_OK;
}

VARIANT ahkFunctionVariant(LPTSTR func, VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10, int sendOrPost);
HRESULT __stdcall CCOMServer::ahkFunction(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (returnVal == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;

	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	for (int i = 0; i < MAX_AHK_THREADS; i++)
	{
		if (g_ahkThreads[i][5] == ThreadID)
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
			break;
		}
	}
	if (!curr_teb)
		return E_FAIL;

	TCHAR buf[MAX_INTEGER_SIZE];
	// CComVariant b ;
	VARIANT b;
	b = ahkFunctionVariant(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName, buf)
		, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, 1);
	VariantInit(returnVal);
	curr_teb->ThreadLocalStoragePointer = tls;
	return VariantCopy(returnVal, &b);
	// return b.Detach(returnVal);			
}


HRESULT __stdcall CCOMServer::ahkPostFunction(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (returnVal == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;

	PMYTEB curr_teb = NULL;
	PVOID tls = NULL;
	for (int i = 0; i < MAX_AHK_THREADS; i++)
	{
		if (g_ahkThreads[i][5] == ThreadID)
		{
			curr_teb = (PMYTEB)NtCurrentTeb();
			tls = curr_teb->ThreadLocalStoragePointer;
			curr_teb->ThreadLocalStoragePointer = (PVOID)g_ahkThreads[i][6];
			break;
		}
	}
	if (!curr_teb)
		return E_FAIL;

	TCHAR buf[MAX_INTEGER_SIZE];
	// CComVariant b ;
	VARIANT b;
	b = ahkFunctionVariant(FuncName.vt == VT_BSTR ? OLE2T(FuncName.bstrVal) : Variant2T(FuncName, buf)
		, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, 0);
	VariantInit(returnVal);
	curr_teb->ThreadLocalStoragePointer = tls;
	return VariantCopy(returnVal, &b);
	// return b.Detach(returnVal);			
}
HRESULT __stdcall CCOMServer::addScript(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT script,/*[in,optional]*/ VARIANT waitexecute,/*[out, retval]*/ UINT_PTR* success)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (success == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_addScript(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script, buf), Variant2I(waitexecute), ThreadID);
	return S_OK;
}
HRESULT __stdcall CCOMServer::ahkExec(/*in*/ VARIANT aThreadID,/*[in]*/ VARIANT script,/*[out, retval]*/ int* success)
{
	USES_CONVERSION;
	DWORD ThreadID = Variant2I(aThreadID);
	if (success == NULL || ThreadID == 0)
		return ERROR_INVALID_PARAMETER;
	TCHAR buf[MAX_INTEGER_SIZE];
	*success = com_ahkExec(script.vt == VT_BSTR ? OLE2T(script.bstrVal) : Variant2T(script, buf), ThreadID);
	return S_OK;
}

#endif