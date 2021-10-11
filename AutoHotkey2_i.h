

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0626 */
/* at Tue Jan 19 04:14:07 2038
 */
/* Compiler settings for source\resources\AutoHotkey2.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0626 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __AutoHotkey2_i_h__
#define __AutoHotkey2_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if _CONTROL_FLOW_GUARD_XFG
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __ICOMServer_FWD_DEFINED__
#define __ICOMServer_FWD_DEFINED__
typedef interface ICOMServer ICOMServer;

#endif 	/* __ICOMServer_FWD_DEFINED__ */


#ifndef __AuotHotkey2COMServer_FWD_DEFINED__
#define __AuotHotkey2COMServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class AuotHotkey2COMServer AuotHotkey2COMServer;
#else
typedef struct AuotHotkey2COMServer AuotHotkey2COMServer;
#endif /* __cplusplus */

#endif 	/* __AuotHotkey2COMServer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ICOMServer_INTERFACE_DEFINED__
#define __ICOMServer_INTERFACE_DEFINED__

/* interface ICOMServer */
/* [unique][nonextensible][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_ICOMServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAB77E75-3E21-4171-A9E2-131179CEEF56")
    ICOMServer : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE NewThread( 
            /* [optional][in] */ VARIANT script,
            /* [optional][in] */ VARIANT params,
            /* [optional][in] */ VARIANT title,
            /* [retval][out] */ unsigned int *ThreadID) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkPause( 
            /* [in] */ VARIANT aThreadID,
            /* [optional][in] */ VARIANT aChangeTo,
            /* [retval][out] */ int *paused) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkReady( 
            /* [in] */ VARIANT aThreadID,
            /* [retval][out] */ int *ready) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindLabel( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT aLabelName,
            /* [retval][out] */ UINT_PTR *pLabel) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkgetvar( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT getVar,
            /* [retval][out] */ VARIANT *returnVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkassign( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT value,
            /* [retval][out] */ int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExecuteLine( 
            /* [in] */ VARIANT aThreadID,
            /* [optional][in] */ VARIANT line,
            /* [optional][in] */ VARIANT aMode,
            /* [optional][in] */ VARIANT wait,
            /* [retval][out] */ UINT_PTR *pLine) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkLabel( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT aLabelName,
            /* [optional][in] */ VARIANT nowait,
            /* [retval][out] */ int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindFunc( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [retval][out] */ UINT_PTR *pFunc) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFunction( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [optional][in] */ VARIANT param1,
            /* [optional][in] */ VARIANT param2,
            /* [optional][in] */ VARIANT param3,
            /* [optional][in] */ VARIANT param4,
            /* [optional][in] */ VARIANT param5,
            /* [optional][in] */ VARIANT param6,
            /* [optional][in] */ VARIANT param7,
            /* [optional][in] */ VARIANT param8,
            /* [optional][in] */ VARIANT param9,
            /* [optional][in] */ VARIANT param10,
            /* [retval][out] */ VARIANT *returnVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkPostFunction( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [optional][in] */ VARIANT param1,
            /* [optional][in] */ VARIANT param2,
            /* [optional][in] */ VARIANT param3,
            /* [optional][in] */ VARIANT param4,
            /* [optional][in] */ VARIANT param5,
            /* [optional][in] */ VARIANT param6,
            /* [optional][in] */ VARIANT param7,
            /* [optional][in] */ VARIANT param8,
            /* [optional][in] */ VARIANT param9,
            /* [optional][in] */ VARIANT param10,
            /* [retval][out] */ VARIANT *returnVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addScript( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT script,
            /* [optional][in] */ VARIANT waitexecute,
            /* [retval][out] */ UINT_PTR *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExec( 
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT script,
            /* [retval][out] */ BOOL *success) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICOMServerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICOMServer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICOMServer * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICOMServer * This);
        
        DECLSPEC_XFGVIRT(IDispatch, GetTypeInfoCount)
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICOMServer * This,
            /* [out] */ UINT *pctinfo);
        
        DECLSPEC_XFGVIRT(IDispatch, GetTypeInfo)
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICOMServer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        DECLSPEC_XFGVIRT(IDispatch, GetIDsOfNames)
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICOMServer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        DECLSPEC_XFGVIRT(IDispatch, Invoke)
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICOMServer * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        DECLSPEC_XFGVIRT(ICOMServer, NewThread)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *NewThread )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT script,
            /* [optional][in] */ VARIANT params,
            /* [optional][in] */ VARIANT title,
            /* [retval][out] */ unsigned int *ThreadID);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkPause)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkPause )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [optional][in] */ VARIANT aChangeTo,
            /* [retval][out] */ int *paused);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkReady)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkReady )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [retval][out] */ int *ready);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkFindLabel)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFindLabel )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT aLabelName,
            /* [retval][out] */ UINT_PTR *pLabel);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkgetvar)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkgetvar )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT getVar,
            /* [retval][out] */ VARIANT *returnVal);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkassign)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkassign )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT value,
            /* [retval][out] */ int *success);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkExecuteLine)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkExecuteLine )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [optional][in] */ VARIANT line,
            /* [optional][in] */ VARIANT aMode,
            /* [optional][in] */ VARIANT wait,
            /* [retval][out] */ UINT_PTR *pLine);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkLabel)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkLabel )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT aLabelName,
            /* [optional][in] */ VARIANT nowait,
            /* [retval][out] */ int *success);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkFindFunc)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFindFunc )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [retval][out] */ UINT_PTR *pFunc);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkFunction)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFunction )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [optional][in] */ VARIANT param1,
            /* [optional][in] */ VARIANT param2,
            /* [optional][in] */ VARIANT param3,
            /* [optional][in] */ VARIANT param4,
            /* [optional][in] */ VARIANT param5,
            /* [optional][in] */ VARIANT param6,
            /* [optional][in] */ VARIANT param7,
            /* [optional][in] */ VARIANT param8,
            /* [optional][in] */ VARIANT param9,
            /* [optional][in] */ VARIANT param10,
            /* [retval][out] */ VARIANT *returnVal);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkPostFunction)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkPostFunction )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT FuncName,
            /* [optional][in] */ VARIANT param1,
            /* [optional][in] */ VARIANT param2,
            /* [optional][in] */ VARIANT param3,
            /* [optional][in] */ VARIANT param4,
            /* [optional][in] */ VARIANT param5,
            /* [optional][in] */ VARIANT param6,
            /* [optional][in] */ VARIANT param7,
            /* [optional][in] */ VARIANT param8,
            /* [optional][in] */ VARIANT param9,
            /* [optional][in] */ VARIANT param10,
            /* [retval][out] */ VARIANT *returnVal);
        
        DECLSPEC_XFGVIRT(ICOMServer, addScript)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addScript )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT script,
            /* [optional][in] */ VARIANT waitexecute,
            /* [retval][out] */ UINT_PTR *success);
        
        DECLSPEC_XFGVIRT(ICOMServer, ahkExec)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkExec )( 
            ICOMServer * This,
            /* [in] */ VARIANT aThreadID,
            /* [in] */ VARIANT script,
            /* [retval][out] */ BOOL *success);
        
        END_INTERFACE
    } ICOMServerVtbl;

    interface ICOMServer
    {
        CONST_VTBL struct ICOMServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICOMServer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICOMServer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICOMServer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICOMServer_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ICOMServer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ICOMServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ICOMServer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ICOMServer_NewThread(This,script,params,title,ThreadID)	\
    ( (This)->lpVtbl -> NewThread(This,script,params,title,ThreadID) ) 

#define ICOMServer_ahkPause(This,aThreadID,aChangeTo,paused)	\
    ( (This)->lpVtbl -> ahkPause(This,aThreadID,aChangeTo,paused) ) 

#define ICOMServer_ahkReady(This,aThreadID,ready)	\
    ( (This)->lpVtbl -> ahkReady(This,aThreadID,ready) ) 

#define ICOMServer_ahkFindLabel(This,aThreadID,aLabelName,pLabel)	\
    ( (This)->lpVtbl -> ahkFindLabel(This,aThreadID,aLabelName,pLabel) ) 

#define ICOMServer_ahkgetvar(This,aThreadID,name,getVar,returnVal)	\
    ( (This)->lpVtbl -> ahkgetvar(This,aThreadID,name,getVar,returnVal) ) 

#define ICOMServer_ahkassign(This,aThreadID,name,value,success)	\
    ( (This)->lpVtbl -> ahkassign(This,aThreadID,name,value,success) ) 

#define ICOMServer_ahkExecuteLine(This,aThreadID,line,aMode,wait,pLine)	\
    ( (This)->lpVtbl -> ahkExecuteLine(This,aThreadID,line,aMode,wait,pLine) ) 

#define ICOMServer_ahkLabel(This,aThreadID,aLabelName,nowait,success)	\
    ( (This)->lpVtbl -> ahkLabel(This,aThreadID,aLabelName,nowait,success) ) 

#define ICOMServer_ahkFindFunc(This,aThreadID,FuncName,pFunc)	\
    ( (This)->lpVtbl -> ahkFindFunc(This,aThreadID,FuncName,pFunc) ) 

#define ICOMServer_ahkFunction(This,aThreadID,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal)	\
    ( (This)->lpVtbl -> ahkFunction(This,aThreadID,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal) ) 

#define ICOMServer_ahkPostFunction(This,aThreadID,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal)	\
    ( (This)->lpVtbl -> ahkPostFunction(This,aThreadID,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal) ) 

#define ICOMServer_addScript(This,aThreadID,script,waitexecute,success)	\
    ( (This)->lpVtbl -> addScript(This,aThreadID,script,waitexecute,success) ) 

#define ICOMServer_ahkExec(This,aThreadID,script,success)	\
    ( (This)->lpVtbl -> ahkExec(This,aThreadID,script,success) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICOMServer_INTERFACE_DEFINED__ */



#ifndef __AutoHotkey2_LIBRARY_DEFINED__
#define __AutoHotkey2_LIBRARY_DEFINED__

/* library AutoHotkey2 */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_AutoHotkey2;

EXTERN_C const CLSID CLSID_AuotHotkey2COMServer;

#ifdef __cplusplus

class DECLSPEC_UUID("1EB0A161-FE26-4C22-A15D-1B6C6F2677B9")
AuotHotkey2COMServer;
#endif
#endif /* __AutoHotkey2_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

unsigned long             __RPC_USER  VARIANT_UserSize64(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal64(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal64(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree64(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


