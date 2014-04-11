

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sun Aug 07 08:49:23 2011
 */
/* Compiler settings for source\ComServer.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __ComServer_h_h__
#define __ComServer_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICOMServer_FWD_DEFINED__
#define __ICOMServer_FWD_DEFINED__
typedef interface ICOMServer ICOMServer;
#endif 	/* __ICOMServer_FWD_DEFINED__ */


#ifndef __CoCOMServer_FWD_DEFINED__
#define __CoCOMServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class CoCOMServer CoCOMServer;
#else
typedef struct CoCOMServer CoCOMServer;
#endif /* __cplusplus */

#endif 	/* __CoCOMServer_FWD_DEFINED__ */


#ifndef __CoCOMServerOptional_FWD_DEFINED__
#define __CoCOMServerOptional_FWD_DEFINED__

#ifdef __cplusplus
typedef class CoCOMServerOptional CoCOMServerOptional;
#else
typedef struct CoCOMServerOptional CoCOMServerOptional;
#endif /* __cplusplus */

#endif 	/* __CoCOMServerOptional_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __LibCOMServer_LIBRARY_DEFINED__
#define __LibCOMServer_LIBRARY_DEFINED__

/* library LibCOMServer */
/* [version][uuid] */ 


DEFINE_GUID(LIBID_LibCOMServer,0xA9863C65,0x8CD4,0x4069,0x89,0x3D,0x3B,0x5A,0x3D,0xDF,0xAE,0x88);

#ifndef __ICOMServer_INTERFACE_DEFINED__
#define __ICOMServer_INTERFACE_DEFINED__

/* interface ICOMServer */
/* [object][oleautomation][dual][uuid] */ 


DEFINE_GUID(IID_ICOMServer,0x04FFE41B,0x8FE9,0x4479,0x99,0x0A,0xB1,0x86,0xEC,0x73,0xF4,0x9C);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04FFE41B-8FE9-4479-990A-B186EC73F49C")
    ICOMServer : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahktextdll( 
            /* [optional][in] */ VARIANT script,
            /* [optional][in] */ VARIANT params,
            /* [retval][out] */ unsigned int *hThread) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkdll( 
            /* [optional][in] */ VARIANT filepath,
            /* [optional][in] */ VARIANT params,
            /* [retval][out] */ unsigned int *hThread) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkPause( 
            /* [optional][in] */ VARIANT aChangeTo,
            /* [retval][out] */ int *paused) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkReady( 
            /* [retval][out] */ int *ready) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindLabel( 
            /* [in] */ VARIANT aLabelName,
            /* [retval][out] */ __int64 *pLabel) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkgetvar( 
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT getVar,
            /* [retval][out] */ VARIANT *returnVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkassign( 
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT value,
            /* [retval][out] */ unsigned int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExecuteLine( 
            /* [optional][in] */ VARIANT line,
            /* [optional][in] */ VARIANT aMode,
            /* [optional][in] */ VARIANT wait,
            /* [retval][out] */ unsigned int *pLine) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkLabel( 
            /* [in] */ VARIANT aLabelName,
            /* [optional][in] */ VARIANT nowait,
            /* [retval][out] */ int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindFunction( 
            /* [in] */ VARIANT FuncName,
            /* [retval][out] */ __int64 *pFunc) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFunction( 
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
            /* [retval][out] */ unsigned int *returnVal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkKey( 
            /* [in] */ VARIANT name,
            /* [retval][out] */ int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addScript( 
            /* [in] */ VARIANT script,
            /* [optional][in] */ VARIANT replace,
            /* [retval][out] */ unsigned int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addFile( 
            /* [in] */ VARIANT filepath,
            /* [optional][in] */ VARIANT aAllowDuplicateInclude,
            /* [optional][in] */ VARIANT aIgnoreLoadFailure,
            /* [retval][out] */ unsigned int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExec( 
            /* [in] */ VARIANT script,
            /* [retval][out] */ int *success) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkTerminate( 
            /* [optional][in] */ VARIANT kill,
            /* [retval][out] */ int *success) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICOMServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICOMServer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICOMServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICOMServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICOMServer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICOMServer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICOMServer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICOMServer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahktextdll )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT script,
            /* [optional][in] */ VARIANT params,
            /* [retval][out] */ unsigned int *hThread);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkdll )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT filepath,
            /* [optional][in] */ VARIANT params,
            /* [retval][out] */ unsigned int *hThread);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkPause )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT aChangeTo,
            /* [retval][out] */ int *paused);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkReady )( 
            ICOMServer * This,
            /* [retval][out] */ int *ready);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFindLabel )( 
            ICOMServer * This,
            /* [in] */ VARIANT aLabelName,
            /* [retval][out] */ __int64 *pLabel);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkgetvar )( 
            ICOMServer * This,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT getVar,
            /* [retval][out] */ VARIANT *returnVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkassign )( 
            ICOMServer * This,
            /* [in] */ VARIANT name,
            /* [optional][in] */ VARIANT value,
            /* [retval][out] */ unsigned int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkExecuteLine )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT line,
            /* [optional][in] */ VARIANT aMode,
            /* [optional][in] */ VARIANT wait,
            /* [retval][out] */ unsigned int *pLine);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkLabel )( 
            ICOMServer * This,
            /* [in] */ VARIANT aLabelName,
            /* [optional][in] */ VARIANT nowait,
            /* [retval][out] */ int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFindFunction )( 
            ICOMServer * This,
            /* [in] */ VARIANT FuncName,
            /* [retval][out] */ __int64 *pFunc);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkFunction )( 
            ICOMServer * This,
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
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkPostFunction )( 
            ICOMServer * This,
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
            /* [retval][out] */ unsigned int *returnVal);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkKey )( 
            ICOMServer * This,
            /* [in] */ VARIANT name,
            /* [retval][out] */ int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addScript )( 
            ICOMServer * This,
            /* [in] */ VARIANT script,
            /* [optional][in] */ VARIANT replace,
            /* [retval][out] */ unsigned int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addFile )( 
            ICOMServer * This,
            /* [in] */ VARIANT filepath,
            /* [optional][in] */ VARIANT aAllowDuplicateInclude,
            /* [optional][in] */ VARIANT aIgnoreLoadFailure,
            /* [retval][out] */ unsigned int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkExec )( 
            ICOMServer * This,
            /* [in] */ VARIANT script,
            /* [retval][out] */ int *success);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ahkTerminate )( 
            ICOMServer * This,
            /* [optional][in] */ VARIANT kill,
            /* [retval][out] */ int *success);
        
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


#define ICOMServer_ahktextdll(This,script,params,hThread)	\
    ( (This)->lpVtbl -> ahktextdll(This,script,params,hThread) ) 

#define ICOMServer_ahkdll(This,filepath,params,hThread)	\
    ( (This)->lpVtbl -> ahkdll(This,filepath,params,hThread) ) 

#define ICOMServer_ahkPause(This,aChangeTo,paused)	\
    ( (This)->lpVtbl -> ahkPause(This,aChangeTo,paused) ) 

#define ICOMServer_ahkReady(This,ready)	\
    ( (This)->lpVtbl -> ahkReady(This,ready) ) 

#define ICOMServer_ahkFindLabel(This,aLabelName,pLabel)	\
    ( (This)->lpVtbl -> ahkFindLabel(This,aLabelName,pLabel) ) 

#define ICOMServer_ahkgetvar(This,name,getVar,returnVal)	\
    ( (This)->lpVtbl -> ahkgetvar(This,name,getVar,returnVal) ) 

#define ICOMServer_ahkassign(This,name,value,success)	\
    ( (This)->lpVtbl -> ahkassign(This,name,value,success) ) 

#define ICOMServer_ahkExecuteLine(This,line,aMode,wait,pLine)	\
    ( (This)->lpVtbl -> ahkExecuteLine(This,line,aMode,wait,pLine) ) 

#define ICOMServer_ahkLabel(This,aLabelName,nowait,success)	\
    ( (This)->lpVtbl -> ahkLabel(This,aLabelName,nowait,success) ) 

#define ICOMServer_ahkFindFunction(This,FuncName,pFunc)	\
    ( (This)->lpVtbl -> ahkFindFunction(This,FuncName,pFunc) ) 

#define ICOMServer_ahkFunction(This,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal)	\
    ( (This)->lpVtbl -> ahkFunction(This,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal) ) 

#define ICOMServer_ahkPostFunction(This,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal)	\
    ( (This)->lpVtbl -> ahkPostFunction(This,FuncName,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,returnVal) ) 

#define ICOMServer_ahkKey(This,name,success)	\
    ( (This)->lpVtbl -> ahkKey(This,name,success) ) 

#define ICOMServer_addScript(This,script,replace,success)	\
    ( (This)->lpVtbl -> addScript(This,script,replace,success) ) 

#define ICOMServer_addFile(This,filepath,aAllowDuplicateInclude,aIgnoreLoadFailure,success)	\
    ( (This)->lpVtbl -> addFile(This,filepath,aAllowDuplicateInclude,aIgnoreLoadFailure,success) ) 

#define ICOMServer_ahkExec(This,script,success)	\
    ( (This)->lpVtbl -> ahkExec(This,script,success) ) 

#define ICOMServer_ahkTerminate(This,kill,success)	\
    ( (This)->lpVtbl -> ahkTerminate(This,kill,success) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICOMServer_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_CoCOMServer,0xC00BCC8C,0x5A04,0x4392,0x87,0x0F,0x20,0xAA,0xE1,0xB9,0x26,0xB2);

#ifdef __cplusplus

class DECLSPEC_UUID("C00BCC8C-5A04-4392-870F-20AAE1B926B2")
CoCOMServer;
#endif

DEFINE_GUID(CLSID_CoCOMServerOptional,0x974318D9,0xA5B2,0x4FE5,0x8A,0xC4,0x33,0xA0,0xC9,0xEB,0xB8,0xB5);

#ifdef __cplusplus

class DECLSPEC_UUID("974318D9-A5B2-4FE5-8AC4-33A0C9EBB8B5")
CoCOMServerOptional;
#endif
#endif /* __LibCOMServer_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


