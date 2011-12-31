
#ifdef _USRDLL
#ifndef MINIDLL
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sat Oct 09 20:41:58 2010
 */
/* Compiler settings for automcomserver.idl:
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


#ifndef __automcomserver_i_h__
#define __automcomserver_i_h__

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


/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __AutoHotkey2_LIBRARY_DEFINED__
#define __AutoHotkey2_LIBRARY_DEFINED__

/* library AutoHotkey */
/* [version][uuid] */ 


DEFINE_GUID(LIBID_AutoHotkey2,0x9c5f743b, 0xa646, 0x4ec9, 0x8c, 0xe1, 0x7, 0x28, 0xa0, 0x7e, 0x7c, 0x21);

#ifndef __ICOMServer_INTERFACE_DEFINED__
#define __ICOMServer_INTERFACE_DEFINED__

/* interface ICOMServer */
/* [object][oleautomation][dual][uuid] */ 


DEFINE_GUID(IID_ICOMServer,0xa58e17b4, 0xf892, 0x4839, 0x8c, 0x46, 0x9f, 0x3c, 0x6, 0x2f, 0xdf, 0x7d);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A58E17B4-F892-4839-8C46-9F3C062FDF7D")
    ICOMServer : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahktextdll( 
           /*in,optional*/VARIANT script,/*in,optional*/VARIANT params, /* [retval][out] */UINT_PTR* hThread) = 0;
        
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkdll( 
           /*in,optional*/VARIANT filepath,/*in,optional*/VARIANT params, /* [retval][out] */UINT_PTR* hThread) = 0;
        
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkPause( 
           /*in,optional*/VARIANT aChangeTo,/* [retval][out] */BOOL* paused) = 0;
        
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkReady( 
           /* [retval][out] */BOOL* ready) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindLabel( 
           /*[in]*/ VARIANT aLabelName,/*[out, retval]*/ UINT_PTR *pLabel) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkgetvar( 
           /*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT getVar,/*[out, retval]*/ VARIANT *returnVal) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkassign( 
           /*[in]*/ VARIANT name,/*[in,optional]*/ VARIANT value,/*[out, retval]*/ unsigned int* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExecuteLine( 
           /*[in,optional]*/ VARIANT line,/*[in,optional]*/ VARIANT aMode,/*[in,optional]*/ VARIANT wait,/*[out, retval]*/ UINT_PTR* pLine) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkLabel( 
           /*[in]*/ VARIANT aLabelName,/*[in,optional]*/ VARIANT nowait,/*[out, retval]*/ BOOL* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFindFunc( 
           /*[in]*/ VARIANT FuncName,/*[out, retval]*/ UINT_PTR *pFunc) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkFunction( 
           /*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ VARIANT* returnVal) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkPostFunction( 
           /*[in]*/ VARIANT FuncName,/*[in,optional]*/ VARIANT param1,/*[in,optional]*/ VARIANT param2,/*[in,optional]*/ VARIANT param3,/*[in,optional]*/ VARIANT param4,/*[in,optional]*/ VARIANT param5,/*[in,optional]*/ VARIANT param6,/*[in,optional]*/ VARIANT param7,/*[in,optional]*/ VARIANT param8,/*[in,optional]*/ VARIANT param9,/*[in,optional]*/ VARIANT param10,/*[out, retval]*/ unsigned int* returnVal) = 0;
	public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkKey( 
           /*[in]*/ VARIANT name,/*[out, retval]*/ BOOL* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addScript( 
           /*[in]*/ VARIANT script,/*[in,optional]*/ VARIANT replace,/*[out, retval]*/ UINT_PTR* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addFile( 
           /*[in]*/ VARIANT filepath,/*[in,optional]*/ VARIANT aAllowDuplicateInclude,/*[in,optional]*/ VARIANT aIgnoreLoadFailure,/*[out, retval]*/ UINT_PTR* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkExec( 
           /*[in]*/ VARIANT script,/*[out, retval]*/ BOOL* success) = 0;
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ahkTerminate( 
           /*[in,optional]*/ VARIANT kill,/*[out, retval]*/ BOOL* success) = 0;
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
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Name )( 
            ICOMServer * This,
            /* [retval][out] */ BSTR *objectname);
        
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


#define ICOMServer_Name(This,objectname)	\
    ( (This)->lpVtbl -> Name(This,objectname) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICOMServer_INTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_CoCOMServer,0xfeeec4ba, 0x4af, 0x45f0, 0xb3, 0x85, 0x72, 0x90, 0xc6, 0x5c, 0xfb, 0x9b);

#ifdef _WIN64
DEFINE_GUID(CLSID_CoCOMServerOptioal,0xf1d0de03, 0x30fd, 0x4326, 0xb3, 0x3f, 0x98, 0x9b, 0xbf, 0xaa, 0x5f, 0xfa);
#else
#ifdef _UNICODE
DEFINE_GUID(CLSID_CoCOMServerOptional,0xec81ebba, 0x6cee, 0x4363, 0xab, 0x77, 0xc0, 0xe5, 0x70, 0x46, 0xaa, 0x89);
#else
DEFINE_GUID(CLSID_CoCOMServerOptional,0xde91ae03, 0xec49, 0x4a4f, 0x85, 0xdc, 0x10, 0xa, 0x74, 0x40, 0x61, 0x3);
#endif
#endif

#ifdef __cplusplus

class DECLSPEC_UUID("FEEEC4BA-04AF-45F0-B385-7290C65CFB9B")
CoCOMServer;

#ifdef _WIN64
class DECLSPEC_UUID("F1D0DE03-30FD-4326-B33F-989BBFAA5FFA")
#else
#ifdef _UNICODE
class DECLSPEC_UUID("EC81EBBA-6CEE-4363-AB77-C0E57046AA89")
#else
class DECLSPEC_UUID("DE91AE03-EC49-4A4F-85DC-100A74406103")
#endif
#endif
CoCOMServerOptional;
#endif
#endif /* __AutoHotkey2_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


#endif
#endif