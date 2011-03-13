#ifdef _USRDLL
#ifndef MINIDLL

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_LibCOMServer,0xa9863c65, 0x8cd4, 0x4069, 0x89, 0x3d, 0x3b, 0x5a, 0x3d, 0xdf, 0xae, 0x88);


MIDL_DEFINE_GUID(IID, IID_ICOMServer,0x4ffe41b, 0x8fe9, 0x4479, 0x99, 0xa, 0xb1, 0x86, 0xec, 0x73, 0xf4, 0x9c);


MIDL_DEFINE_GUID(CLSID, CLSID_CoCOMServer,0xc00bcc8c, 0x5a04, 0x4392, 0x87, 0xf, 0x20, 0xaa, 0xe1, 0xb9, 0x26, 0xb2);

#ifdef _WIN64
MIDL_DEFINE_GUID(CLSID, CLSID_CoCOMServerOptional,0x38D00012,0xDC83,0x4E17,0x9B,0xAD,0xD9,0xDD,0x97,0x90,0x25,0x80);
#else
#ifdef _UNICODE
MIDL_DEFINE_GUID(CLSID, CLSID_CoCOMServerOptional,0xC58DCD96,0x1D6F,0x4F85,0xB5,0x55,0x02,0xB7,0xF2,0x1F,0x5C,0xAF);
#else
MIDL_DEFINE_GUID(CLSID, CLSID_CoCOMServerOptional,0x974318D9,0xA5B2,0x4FE5,0x8A,0xC4,0x33,0xA0,0xC9,0xEB,0xB8,0xB5);
#endif
#endif

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif
#endif