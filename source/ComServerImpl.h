#ifdef _USRDLL
#ifndef MINIDLL

#pragma once

// ICOMServer interface declaration ///////////////////////////////////////////
//
//

class CoCOMServer : public ICOMServer
{
	// Construction
public:
	CoCOMServer();
	~CoCOMServer();

	// IUnknown implementation
	//
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;
	virtual ULONG __stdcall AddRef() ;
	virtual ULONG __stdcall Release() ;

	//IDispatch implementation
	virtual HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo);
	virtual HRESULT __stdcall GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	virtual HRESULT __stdcall GetIDsOfNames(REFIID riid, 
		LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid);
	virtual HRESULT __stdcall Invoke(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr);


	// ICOMServer implementation
	//
	virtual HRESULT __stdcall ahktextdll(/*in,optional*/VARIANT script,/*in,optional*/VARIANT options,/*in,optional*/VARIANT params, /*out*/unsigned int* hThread);
	virtual HRESULT __stdcall ahkdll(/*in,optional*/VARIANT filepath,/*in,optional*/VARIANT options,/*in,optional*/VARIANT params, /*out*/unsigned int* hThread);
	virtual HRESULT __stdcall ahkPause(/*in,optional*/VARIANT aChangeTo, /*out*/BOOL* paused);
	virtual HRESULT __stdcall ahkReady(/*out*/BOOL* ready);
	virtual HRESULT __stdcall ahkFindLabel(/*[in]*/ VARIANT aLabelName,/*[out, retval]*/ unsigned int* pLabel);
    virtual HRESULT __stdcall ahkgetvar(/*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT getVar,/*[out, retval]*/ VARIANT *returnVal);
    virtual HRESULT __stdcall ahkassign(/*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT value,/*[out, retval]*/ unsigned int* success);
    virtual HRESULT __stdcall ahkExecuteLine(/*[in,optional]*/ VARIANT line,/*[in,optional]*/ VARIANT aMode,/*[in,optional]*/ VARIANT wait,/*[out, retval]*/ unsigned int* pLine);
    virtual HRESULT __stdcall ahkLabel(/*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ BOOL* success);
    virtual HRESULT __stdcall ahkFindFunc(/*[in]*/ VARIANT FuncName,/*[out, retval]*/ unsigned int* pFunc);
    virtual HRESULT __stdcall ahkFunction(/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal);
    virtual HRESULT __stdcall ahkPostFunction(/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ unsigned int* returnVal);
	virtual HRESULT __stdcall ahkKey(/*[in]*/ VARIANT name,/*[out, retval]*/ BOOL* success);
    virtual HRESULT __stdcall addScript(/*[in]*/ VARIANT script,/*[in,optional]*/ VARIANT aExecute,/*[out, retval]*/ unsigned int* success);
    virtual HRESULT __stdcall addFile(/*[in]*/ VARIANT filepath,/*[in,optional]*/ VARIANT aAllowDuplicateInclude,/*[in,optional]*/ VARIANT aIgnoreLoadFailure,/*[out, retval]*/ unsigned int* success);
    virtual HRESULT __stdcall ahkExec(/*[in]*/ VARIANT script,/*[out, retval]*/ BOOL* success);
    virtual HRESULT __stdcall ahkTerminate(/*[in,optional]*/ VARIANT kill,/*[out, retval]*/ BOOL* success);

private:

	HRESULT LoadTypeInfo(ITypeInfo ** pptinfo, const CLSID& libid, const CLSID& iid, LCID lcid);
	
	// Reference count
	long		m_cRef ;
	LPTYPEINFO	m_ptinfo; // pointer to type-library
};






///////////////////////////////////////////////////////////
//
// Class factory
//
class CFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;         
	virtual ULONG   __stdcall AddRef() ;
	virtual ULONG   __stdcall Release() ;

	// Interface IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
	                                         const IID& iid,
	                                         void** ppv) ;
	virtual HRESULT __stdcall LockServer(BOOL bLock) ; 

	// Constructor
	CFactory() : m_cRef(1) {}

	// Destructor
	~CFactory() {;}

private:
	long m_cRef ;
} ;
#endif
#endif