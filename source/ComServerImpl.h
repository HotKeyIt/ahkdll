#ifdef _USRDLL

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
	virtual HRESULT __stdcall ahktextdll(/*in,optional*/VARIANT script,/*in,optional*/VARIANT params,/*in,optional*/VARIANT title, /*out*/UINT_PTR* hThread);
	virtual HRESULT __stdcall ahkdll(/*in,optional*/VARIANT filepath,/*in,optional*/VARIANT params,/*in,optional*/VARIANT title, /*out*/UINT_PTR* hThread);
	virtual HRESULT __stdcall ahkPause(/*in,optional*/VARIANT aChangeTo, /*out*/int* paused);
	virtual HRESULT __stdcall ahkReady(/*out*/int* ready);
	virtual HRESULT __stdcall ahkFindLabel(/*[in]*/ VARIANT aLabelName,/*[out, retval]*/ UINT_PTR *pLabel);
    virtual HRESULT __stdcall ahkgetvar(/*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT getVar,/*[out, retval]*/ VARIANT *returnVal);
    virtual HRESULT __stdcall ahkassign(/*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT value,/*[out, retval]*/ int* success);
    virtual HRESULT __stdcall ahkExecuteLine(/*[in,optional]*/ VARIANT line,/*[in,optional]*/ VARIANT aMode,/*[in,optional]*/ VARIANT wait,/*[out, retval]*/ UINT_PTR* pLine);
    virtual HRESULT __stdcall ahkLabel(/*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ int* success);
    virtual HRESULT __stdcall ahkFindFunc(/*[in]*/ VARIANT FuncName,/*[out, retval]*/ UINT_PTR *pFunc);
    virtual HRESULT __stdcall ahkFunction(/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal);
    virtual HRESULT __stdcall ahkPostFunction(/*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ int* returnVal);
    virtual HRESULT __stdcall addScript(/*[in]*/ VARIANT script,/*[in,optional]*/ VARIANT waitexecute,/*[out, retval]*/ UINT_PTR* success);
    virtual HRESULT __stdcall addFile(/*[in]*/ VARIANT filepath,/*[in,optional]*/ VARIANT waitexecute,/*[out, retval]*/ UINT_PTR* success);
    virtual HRESULT __stdcall ahkExec(/*[in]*/ VARIANT script,/*[out, retval]*/ int* success);
    virtual HRESULT __stdcall ahkTerminate(/*[in,optional]*/ VARIANT kill,/*[out, retval]*/ int* success);
    virtual HRESULT __stdcall ahkReload(/*[in]*/ VARIANT timeout);
	virtual HRESULT __stdcall ahkIsUnicode(/*out*/int* IsUnicode);

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