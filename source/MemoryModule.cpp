/*
 * Memory DLL loading code
 * Version 0.0.4
 *
 * Copyright (c) 2004-2015 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is MemoryModule.c
 *
 * The Initial Developer of the Original Code is Joachim Bauch.
 *
 * Portions created by Joachim Bauch are Copyright (C) 2004-2015
 * Joachim Bauch. All Rights Reserved.
 *
 *
 * THeller: Added binary search in MemoryGetProcAddress function
 * (#define USE_BINARY_SEARCH to enable it).  This gives a very large
 * speedup for libraries that exports lots of functions.
 *
 * These portions are Copyright (C) 2013 Thomas Heller.
 */

#include "stdafx.h" // pre-compiled headers
#include "MemoryModule.h"
#include "globaldata.h" // for access to many global vars
#include "application.h"

#ifdef _WIN64
#define POINTER_TYPE ULONGLONG
#else
#define POINTER_TYPE DWORD
#endif

#include <windows.h>
#include <winnt.h>
#include <stddef.h>
#include <stdint.h>
#include <tchar.h>
#include <winternl.h>
#include <process.h>

//#include "MinHook/MinHook.h"

#include "MemoryModule.h"
#include "MinHook.h"

#if defined _M_X64
#ifdef STATICLIBRARY
#pragma comment(lib, "MT_libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x64.lib")
#endif
#elif defined _M_IX86
#ifdef STATICLIBRARY
#pragma comment(lib, "MT_libMinHook.x86.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif
#endif

#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#if _MSC_VER
// Disable warning about data -> function pointer conversion
#pragma warning(disable:4055)
 // C4244: conversion from 'uintptr_t' to 'DWORD', possible loss of data.
#pragma warning(error: 4244)
// C4267: conversion from 'size_t' to 'int', possible loss of data.
#pragma warning(error: 4267)

#define inline __inline
#endif

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
// Vista SDKs no longer define IMAGE_SIZEOF_BASE_RELOCATION!?
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))
#endif

typedef HANDLE (WINAPI * MyCreateActCtx)(PCACTCTXA);
typedef HANDLE (WINAPI * MyDeactivateActCtx)(DWORD,ULONG_PTR);
typedef BOOL (WINAPI * MyActivateActCtx)(HANDLE,ULONG_PTR*);
HMODULE libkernel32 = LoadLibrary(_T("kernel32.dll"));
MyCreateActCtx _CreateActCtxA = (MyCreateActCtx)GetProcAddress(libkernel32,"CreateActCtxA");
MyDeactivateActCtx _DeactivateActCtx = (MyDeactivateActCtx)GetProcAddress(libkernel32,"DeactivateActCtx");
MyActivateActCtx _ActivateActCtx = (MyActivateActCtx)GetProcAddress(libkernel32,"ActivateActCtx");
#ifdef _WIN64
#define HOST_MACHINE IMAGE_FILE_MACHINE_AMD64
#else
#define HOST_MACHINE IMAGE_FILE_MACHINE_I386
#endif

HMEMORYMODULE currentModuleStart;
PVOID currentModuleEnd;

#define GET_HEADER_DICTIONARY(module, idx)  &(module)->headers->OptionalHeader.DataDirectory[idx]

// hook RtlPcToFileHeader
HANDLE hHeap;
PHOOK_ENTRY pHook = NULL;

typedef struct _MY_LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	WORD LoadCount;
	WORD TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	_ACTIVATION_CONTEXT * EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
} MY_LDR_DATA_TABLE_ENTRY, *PMY_LDR_DATA_TABLE_ENTRY;

typedef struct _MY_PEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[2];
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
} MY_PEB_LDR_DATA, *PMY_PEB_LDR_DATA;


typedef struct _MYPEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PMY_PEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID Reserved4[3];
	PVOID AtlThunkSListPtr;
	PVOID Reserved5;
	ULONG Reserved6;
	PVOID Reserved7;
	ULONG Reserved8;
	ULONG AtlThunkSListPtr32;
	PVOID Reserved9[45];
	BYTE Reserved10[96];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved11[128];
	PVOID Reserved12[1];
	ULONG SessionId;
} MYPEB, *PMYPEB;

PVOID NTAPI HookRtlPcToFileHeader(IN PVOID PcValue, PVOID* BaseOfImage)
{
	if (PcValue >= currentModuleStart && PcValue < currentModuleEnd)
		return *BaseOfImage = currentModuleStart;
	PLIST_ENTRY ModuleListHead;
	PLIST_ENTRY Entry;
	PMY_LDR_DATA_TABLE_ENTRY Module;
	PVOID ImageBase = NULL;

	PMYPEB mypeb;
#ifdef _M_IX86 // compiles for x86
	mypeb = (PMYPEB)(__readfsdword(0x30)); //PEB
#elif _M_AMD64 // compiles for x64
	mypeb = (PMYPEB)(__readgsqword(0x60)); //PEB
#endif
	/*
	PCRITICAL_SECTION aLoaderLock; // So no other module can be loaded, expecially due to hooked _RtlPcToFileHeader
#ifdef _M_IX86 // compiles for x86
	aLoaderLock = *(PCRITICAL_SECTION*)(__readfsdword(0x30) + 0xA0); //PEB->LoaderLock
#elif _M_AMD64 // compiles for x64
	aLoaderLock = *(PCRITICAL_SECTION*)(__readgsqword(0x60) + 0x110); //PEB->LoaderLock //0x60 because offset is doubled in 64bit
#endif

	EnterCriticalSection(aLoaderLock);
	*/
	// Enter and Leave Critical Section is done in MemoryLoadLibraryEx
	ModuleListHead = &mypeb->Ldr->InLoadOrderModuleList;
	Entry = ModuleListHead->Flink;
	while (Entry != ModuleListHead)
	{
		Module = CONTAINING_RECORD(Entry, MY_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if ((ULONG_PTR)PcValue >= (ULONG_PTR)Module->DllBase &&
			(ULONG_PTR)PcValue < (ULONG_PTR)Module->DllBase + Module->SizeOfImage)
		{
			ImageBase = Module->DllBase;
			break;
		}
		Entry = Entry->Flink;
	}
	//LeaveCriticalSection(aLoaderLock);

	*BaseOfImage = ImageBase;
	return ImageBase;
}

static inline uintptr_t
AlignValueDown(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1);
}

static inline LPVOID
AlignAddressDown(LPVOID address, uintptr_t alignment) {
    return (LPVOID) AlignValueDown((uintptr_t) address, alignment);
}

static inline size_t
AlignValueUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static inline void*
OffsetPointer(void* data, ptrdiff_t offset) {
    return (void*) ((uintptr_t) data + offset);
}

static inline void
OutputLastError(const char *msg)
{
#ifndef DEBUG_OUTPUT
    UNREFERENCED_PARAMETER(msg);
#else
    LPVOID tmp;
    char *tmpmsg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
    tmpmsg = (char *)LocalAlloc(LPTR, strlen(msg) + strlen(tmp) + 3);
    sprintf(tmpmsg, "%s: %s", msg, tmp);
    OutputDebugString(tmpmsg);
    LocalFree(tmpmsg);
    LocalFree(tmp);
#endif
}

#ifdef _WIN64
static void
FreePointerList(POINTER_LIST *head, CustomFreeFunc freeMemory, void *userdata)
{
    POINTER_LIST *node = head;
    while (node) {
        POINTER_LIST *next;
        freeMemory(node->address, 0, MEM_RELEASE, userdata);
        next = node->next;
        free(node);
        node = next;
    }
}
#endif

static BOOL
CheckSize(size_t size, size_t expected) {
    if (size < expected) {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    return TRUE;
}

static BOOL
CopySections(const unsigned char *data, size_t size, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
{
    int i, section_size;
    unsigned char *codeBase = module->codeBase;
    unsigned char *dest;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
    for (i=0; i<module->headers->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData == 0) {
            // section doesn't contain data in the dll itself, but may define
            // uninitialized data
            section_size = old_headers->OptionalHeader.SectionAlignment;
            if (section_size > 0) {
                dest = (unsigned char *)module->alloc(codeBase + section->VirtualAddress,
                    section_size,
                    MEM_COMMIT,
                    PAGE_READWRITE,
                    module->userdata);
                if (dest == NULL) {
                    return FALSE;
                }

                // Always use position from file to support alignments smaller
                // than page size (allocation above will align to page size).
                dest = codeBase + section->VirtualAddress;
                // NOTE: On 64bit systems we truncate to 32bit here but expand
                // again later when "PhysicalAddress" is used.
                section->Misc.PhysicalAddress = (DWORD) ((uintptr_t) dest & 0xffffffff);
                memset(dest, 0, section_size);
            }

            // section is empty
            continue;
        }

        if (size && !CheckSize(size, section->PointerToRawData + section->SizeOfRawData)) {
            return FALSE;
        }

        // commit memory block and copy data from dll
        dest = (unsigned char *)module->alloc(codeBase + section->VirtualAddress,
                            section->SizeOfRawData,
                            MEM_COMMIT,
                            PAGE_READWRITE,
                            module->userdata);
        if (dest == NULL) {
            return FALSE;
        }

        // Always use position from file to support alignments smaller
        // than page size (allocation above will align to page size).
        dest = codeBase + section->VirtualAddress;
        memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
        // NOTE: On 64bit systems we truncate to 32bit here but expand
        // again later when "PhysicalAddress" is used.
        section->Misc.PhysicalAddress = (DWORD) ((uintptr_t) dest & 0xffffffff);
    }

    return TRUE;
}

// Protection flags for memory pages (Executable, Readable, Writeable)
static int ProtectionFlags[2][2][2] = {
    {
        // not executable
        {PAGE_NOACCESS, PAGE_WRITECOPY},
        {PAGE_READONLY, PAGE_READWRITE},
    }, {
        // executable
        {PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
        {PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
    },
};

static SIZE_T
GetRealSectionSize(PMEMORYMODULE module, PIMAGE_SECTION_HEADER section) {
    DWORD size = section->SizeOfRawData;
    if (size == 0) {
        if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfInitializedData;
        } else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfUninitializedData;
        }
    }
    return (SIZE_T) size;
}

static BOOL
FinalizeSection(PMEMORYMODULE module, PSECTIONFINALIZEDATA sectionData) {
    DWORD protect, oldProtect;
    BOOL executable;
    BOOL readable;
    BOOL writeable;

    if (sectionData->size == 0) {
        return TRUE;
    }

    if (sectionData->characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
        // section is not needed any more and can safely be freed
        if (sectionData->address == sectionData->alignedAddress &&
            (sectionData->last ||
             module->headers->OptionalHeader.SectionAlignment == module->pageSize ||
             (sectionData->size % module->pageSize) == 0)
           ) {
            // Only allowed to decommit whole pages
            module->free(sectionData->address, sectionData->size, MEM_DECOMMIT, module->userdata);
        }
        return TRUE;
    }

    // determine protection flags based on characteristics
    executable = (sectionData->characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
    readable =   (sectionData->characteristics & IMAGE_SCN_MEM_READ) != 0;
    writeable =  (sectionData->characteristics & IMAGE_SCN_MEM_WRITE) != 0;
    protect = ProtectionFlags[executable][readable][writeable];
    if (sectionData->characteristics & IMAGE_SCN_MEM_NOT_CACHED) {
        protect |= PAGE_NOCACHE;
    }

    // change memory access flags
    if (VirtualProtect(sectionData->address, sectionData->size, protect, &oldProtect) == 0) {
        OutputLastError("Error protecting memory page");
        return FALSE;
    }

    return TRUE;
}

static BOOL
FinalizeSections(PMEMORYMODULE module)
{
    int i;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
#ifdef _WIN64
    // "PhysicalAddress" might have been truncated to 32bit above, expand to
    // 64bits again.
    uintptr_t imageOffset = ((uintptr_t) module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
    static const uintptr_t imageOffset = 0;
#endif
    SECTIONFINALIZEDATA sectionData;
    sectionData.address = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
    sectionData.alignedAddress = AlignAddressDown(sectionData.address, module->pageSize);
    sectionData.size = GetRealSectionSize(module, section);
    sectionData.characteristics = section->Characteristics;
    sectionData.last = FALSE;
    section++;

    // loop through all sections and change access flags
    for (i=1; i<module->headers->FileHeader.NumberOfSections; i++, section++) {
        LPVOID sectionAddress = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
        LPVOID alignedAddress = AlignAddressDown(sectionAddress, module->pageSize);
        SIZE_T sectionSize = GetRealSectionSize(module, section);
        // Combine access flags of all sections that share a page
        // TODO(fancycode): We currently share flags of a trailing large section
        //   with the page of a first small section. This should be optimized.
        if (sectionData.alignedAddress == alignedAddress || (uintptr_t) sectionData.address + sectionData.size > (uintptr_t) alignedAddress) {
            // Section shares page with previous
            if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 || (sectionData.characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
                sectionData.characteristics = (sectionData.characteristics | section->Characteristics) & ~IMAGE_SCN_MEM_DISCARDABLE;
            } else {
                sectionData.characteristics |= section->Characteristics;
            }
            sectionData.size = (((uintptr_t)sectionAddress) + ((uintptr_t) sectionSize)) - (uintptr_t) sectionData.address;
            continue;
        }

        if (!FinalizeSection(module, &sectionData)) {
            return FALSE;
        }
        sectionData.address = sectionAddress;
        sectionData.alignedAddress = alignedAddress;
        sectionData.size = sectionSize;
        sectionData.characteristics = section->Characteristics;
    }
    sectionData.last = TRUE;
    if (!FinalizeSection(module, &sectionData)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL
ExecuteTLS(PMEMORYMODULE module)
{
    unsigned char *codeBase = module->codeBase;
    PIMAGE_TLS_DIRECTORY tls;
    PIMAGE_TLS_CALLBACK* callback;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_TLS);
    if (directory->VirtualAddress == 0) {
        return TRUE;
    }

    tls = (PIMAGE_TLS_DIRECTORY) (codeBase + directory->VirtualAddress);
    callback = (PIMAGE_TLS_CALLBACK *) tls->AddressOfCallBacks;
    if (callback) {
        while (*callback) {
            (*callback)((LPVOID) codeBase, DLL_PROCESS_ATTACH, NULL);
            callback++;
        }
    }
    return TRUE;
}

static BOOL
PerformBaseRelocation(PMEMORYMODULE module, ptrdiff_t delta)
{
    unsigned char *codeBase = module->codeBase;
    PIMAGE_BASE_RELOCATION relocation;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
    if (directory->Size == 0) {
        return (delta == 0);
    }

    relocation = (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);
    for (; relocation->VirtualAddress > 0; ) {
        DWORD i;
        unsigned char *dest = codeBase + relocation->VirtualAddress;
        unsigned short *relInfo = (unsigned short*) OffsetPointer(relocation, IMAGE_SIZEOF_BASE_RELOCATION);
        for (i=0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++) {
            // the upper 4 bits define the type of relocation
            int type = *relInfo >> 12;
            // the lower 12 bits define the offset
            int offset = *relInfo & 0xfff;

            switch (type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                // skip relocation
                break;

            case IMAGE_REL_BASED_HIGHLOW:
                // change complete 32 bit address
                {
                    DWORD *patchAddrHL = (DWORD *) (dest + offset);
                    *patchAddrHL += (DWORD) delta;
                }
                break;

#ifdef _WIN64
            case IMAGE_REL_BASED_DIR64:
                {
                    ULONGLONG *patchAddr64 = (ULONGLONG *) (dest + offset);
                    *patchAddr64 += (ULONGLONG) delta;
                }
                break;
#endif

            default:
                //printf("Unknown relocation: %d\n", type);
                break;
            }
        }

		// flush instruction cache to avoid executing stale code after performing relocations
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dest, module->pageSize);

        // advance to next relocation block
        relocation = (PIMAGE_BASE_RELOCATION) OffsetPointer(relocation, relocation->SizeOfBlock);
    }
    return TRUE;
}

static BOOL
BuildImportTable(PMEMORYMODULE module)
{
    unsigned char *codeBase = module->codeBase;
    ULONG_PTR lpCookie = NULL;
    BOOL result = TRUE;

    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
    if (directory->Size == 0) {
        return TRUE;
    }
    PIMAGE_DATA_DIRECTORY resource = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_RESOURCE);
    
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);
    // Following will be used to resolve manifest in module
    if (resource && resource->Size)
    {
        PIMAGE_RESOURCE_DIRECTORY resDir = (PIMAGE_RESOURCE_DIRECTORY)(codeBase + resource->VirtualAddress);
        PIMAGE_RESOURCE_DIRECTORY resDirTemp;
        PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) ((char*)resDir + sizeof(IMAGE_RESOURCE_DIRECTORY));
        PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntryTemp;
        PIMAGE_RESOURCE_DATA_ENTRY resDataEntry;

        // ACTCTX Structure, not used members must be set to 0!
        ACTCTXA actctx ={0,0,0,0,0,0,0,0,0};
        actctx.cbSize =  sizeof(actctx);
        HANDLE hActCtx;
        
        // Path to temp directory + our temporary file name
        CHAR buf[MAX_PATH];
        DWORD tempPathLength = GetTempPathA(MAX_PATH, buf);
        memcpy(buf + tempPathLength,"AutoHotkey.MemoryModule.temp.manifest",38);
        actctx.lpSource = buf;

        // Enumerate Resources
        int i = 0;
        if (_CreateActCtxA != NULL)
        for (;i < resDir->NumberOfIdEntries + resDir->NumberOfNamedEntries;i++)
        {
            // Resolve current entry
            resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((char*)resDir + sizeof(IMAGE_RESOURCE_DIRECTORY) + (i*sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY)));
            
            // If entry is directory and Id is 24 = RT_MANIFEST
            if (resDirEntry->DataIsDirectory && resDirEntry->Id == 24)
            {
                //resDirTemp = (PIMAGE_RESOURCE_DIRECTORY)((char*)resDir + (resDirEntry->OffsetToDirectory));
                resDirEntryTemp = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((char*)resDir + (resDirEntry->OffsetToDirectory) + sizeof(IMAGE_RESOURCE_DIRECTORY));
                resDirTemp = (PIMAGE_RESOURCE_DIRECTORY) ((char*)resDir + (resDirEntryTemp->OffsetToDirectory));
                resDirEntryTemp = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((char*)resDir + (resDirEntryTemp->OffsetToDirectory) + sizeof(IMAGE_RESOURCE_DIRECTORY));
                resDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY) ((char*)resDir + (resDirEntryTemp->OffsetToData));
                
                // Write manifest to temportary file
                // Using FILE_ATTRIBUTE_TEMPORARY will avoid writing it to disk
                // It will be deleted after CreateActCtx has been called.
                HANDLE hFile = CreateFileA(buf,GENERIC_WRITE,NULL,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
#if DEBUG_OUTPUT
                    OutputDebugStringA("CreateFile failed.\n");
#endif
                    break; //failed to create file, continue and try loading without CreateActCtx
                }
                DWORD byteswritten = 0;
                WriteFile(hFile,(codeBase + resDataEntry->OffsetToData),resDataEntry->Size,&byteswritten,NULL);
                CloseHandle(hFile);
                if (byteswritten == 0)
                {
#if DEBUG_OUTPUT
                    OutputDebugStringA("WriteFile failed.\n");
#endif
                    break; //failed to write data, continue and try loading
                }
                
                hActCtx = _CreateActCtxA(&actctx);

                // Open file and automatically delete on CloseHandle (FILE_FLAG_DELETE_ON_CLOSE)
                hFile = CreateFileA(buf,GENERIC_WRITE,FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL);
                CloseHandle(hFile);

                if (hActCtx == INVALID_HANDLE_VALUE)
                    break; //failed to create context, continue and try loading

                _ActivateActCtx(hActCtx,&lpCookie); // Don't care if this fails since we would countinue anyway
                break; // Break since a dll can have only 1 manifest
            }
        }
    }
    for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++) {
        uintptr_t *thunkRef;
        FARPROC *funcRef;
#ifndef _USRDLL
        //HRSRC hResource;
#endif
        HCUSTOMMODULE *tmp;
        HCUSTOMMODULE handle = NULL;
        char *isMsvcr = NULL;
        /*if (g_hMSVCR != NULL && (isMsvcr = strstr((LPSTR)(codeBase + importDesc->Name), "MSVCR100.dll")))
        {
            handle = g_hMSVCR; //GetModuleHandle(_T("MSVCRT.dll"));
            if (tmp == NULL)
                tmp = (HCUSTOMMODULE *)malloc((sizeof(HCUSTOMMODULE)));
            if (tmp == NULL) {
                SetLastError(ERROR_OUTOFMEMORY);
                result = 0;
                break;
            }
            module->modules = tmp;
            module->modules[0] = handle;
        }
		else
        else*/
            handle = module->loadLibrary((LPCSTR) (codeBase + importDesc->Name), module->userdata);

        if (handle == NULL) {
            SetLastError(ERROR_MOD_NOT_FOUND);
            result = FALSE;
            break;
        }
        if (!isMsvcr)
        {
			if (module->numModules == 0)
				tmp = (HCUSTOMMODULE *)HeapAlloc(module->heapmodules, 0, sizeof(HCUSTOMMODULE));
			else
				tmp = (HCUSTOMMODULE *)HeapReAlloc(module->heapmodules, 0, module->modules, (module->numModules + 1)*(sizeof(HCUSTOMMODULE)));
            if (tmp == NULL) {
                module->freeLibrary(handle, module->userdata);
                SetLastError(ERROR_OUTOFMEMORY);
                result = 0;
                break;
            }
            module->modules = tmp;
            //if (module->numModules == 1)
            //    module->modules[0] = NULL;
            module->modules[module->numModules++] = handle;
        }

        if (importDesc->OriginalFirstThunk) {
            thunkRef = (uintptr_t *) (codeBase + importDesc->OriginalFirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        } else {
            // no hint table
            thunkRef = (uintptr_t *) (codeBase + importDesc->FirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        }
        for (; *thunkRef; thunkRef++, funcRef++) {
            if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                if (!isMsvcr)
                    *funcRef = module->getProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef), module->userdata);
                else
                    *funcRef = MemoryGetProcAddress(handle, (LPCSTR)IMAGE_ORDINAL(*thunkRef));
            } else {
                PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
                if (!isMsvcr)
                    *funcRef = module->getProcAddress(handle, (LPCSTR)&thunkData->Name, module->userdata);
                else
                    *funcRef = MemoryGetProcAddress(handle, (LPCSTR)&thunkData->Name);
            }
            if (*funcRef == 0) {
                result = FALSE;
                break;
            }
        }

        if (!result) {
            module->freeLibrary(handle, module->userdata);
            SetLastError(ERROR_PROC_NOT_FOUND);
            break;
        }
    }
    if (_DeactivateActCtx && lpCookie)
        _DeactivateActCtx(NULL,lpCookie);
    return result;
}

LPVOID MemoryDefaultAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata)
{
	UNREFERENCED_PARAMETER(userdata);
	return VirtualAlloc(address, size, allocationType, protect);
}

BOOL MemoryDefaultFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType, void* userdata)
{
	UNREFERENCED_PARAMETER(userdata);
	return VirtualFree(lpAddress, dwSize, dwFreeType);
}

HCUSTOMMODULE MemoryDefaultLoadLibrary(LPCSTR filename, void *userdata)
{
    HMODULE result;
    UNREFERENCED_PARAMETER(userdata);
    result = LoadLibraryA(filename);
    if (result == NULL) {
        return NULL;
    }

    return (HCUSTOMMODULE) result;
}

FARPROC MemoryDefaultGetProcAddress(HCUSTOMMODULE module, LPCSTR name, void *userdata)
{
    UNREFERENCED_PARAMETER(userdata);
    return (FARPROC) GetProcAddress((HMODULE) module, name);
}

void MemoryDefaultFreeLibrary(HCUSTOMMODULE module, void *userdata)
{
    UNREFERENCED_PARAMETER(userdata);
    FreeLibrary((HMODULE) module);
}

HMEMORYMODULE MemoryLoadLibrary(const void *data, size_t size, bool callentry)
{
    return MemoryLoadLibraryEx(data, size, MemoryDefaultAlloc, MemoryDefaultFree, MemoryDefaultLoadLibrary, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL, callentry);
}

HMEMORYMODULE MemoryLoadLibraryEx(const void *data, size_t size,
    CustomAllocFunc allocMemory,
    CustomFreeFunc freeMemory,
    CustomLoadLibraryFunc loadLibrary,
    CustomGetProcAddressFunc getProcAddress,
    CustomFreeLibraryFunc freeLibrary,
    void *userdata, bool callentry)
{
    PMEMORYMODULE result = NULL;
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS old_header;
    unsigned char *code, *headers;
    ptrdiff_t locationDelta;
    // SYSTEM_INFO sysInfo;
    PIMAGE_SECTION_HEADER section;
    DWORD i;
    size_t optionalSectionSize;
    size_t lastSectionEnd = 0;
    size_t alignedImageSize;
#ifdef _WIN64
    POINTER_LIST *blockedMemory = NULL;
#endif

    if (size && !CheckSize(size, sizeof(IMAGE_DOS_HEADER))) {
        return NULL;
    }
    dos_header = (PIMAGE_DOS_HEADER)data;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    if (size && !CheckSize(size, dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS))) {
        return NULL;
    }
    old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *)(data))[dos_header->e_lfanew];
    if (old_header->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    if (old_header->FileHeader.Machine != HOST_MACHINE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    if (old_header->OptionalHeader.SectionAlignment & 1) {
        // Only support section alignments that are a multiple of 2
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    section = IMAGE_FIRST_SECTION(old_header);
    optionalSectionSize = old_header->OptionalHeader.SectionAlignment;
    for (i=0; i<old_header->FileHeader.NumberOfSections; i++, section++) {
        size_t endOfSection;
        if (section->SizeOfRawData == 0) {
            // Section without data in the DLL
            endOfSection = section->VirtualAddress + optionalSectionSize;
        } else {
            endOfSection = section->VirtualAddress + section->SizeOfRawData;
        }

        if (endOfSection > lastSectionEnd) {
            lastSectionEnd = endOfSection;
        }
    }

    // GetNativeSystemInfo(&sysInfo); // HotKeyIt replace sysInfo.dwPageSize by SectionAlignment
    alignedImageSize = AlignValueUp(old_header->OptionalHeader.SizeOfImage, old_header->OptionalHeader.SectionAlignment);
    if (alignedImageSize != AlignValueUp(lastSectionEnd, old_header->OptionalHeader.SectionAlignment)) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // reserve memory for image of library
    // XXX: is it correct to commit the complete memory region at once?
    //      calling DllEntry raises an exception if we don't...
    code = (unsigned char *)allocMemory((LPVOID)(old_header->OptionalHeader.ImageBase),
        alignedImageSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE,
        userdata);

    if (code == NULL) {
        // try to allocate memory at arbitrary position
        code = (unsigned char *)allocMemory(NULL,
            alignedImageSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE,
            userdata);
        if (code == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

#ifdef _WIN64
    // Memory block may not span 4 GB boundaries.
    while ((((uintptr_t) code) >> 32) < (((uintptr_t) (code + alignedImageSize)) >> 32)) {
        POINTER_LIST *node = (POINTER_LIST*) malloc(sizeof(POINTER_LIST));
        if (!node) {
            freeMemory(code, 0, MEM_RELEASE, userdata);
            FreePointerList(blockedMemory, freeMemory, userdata);
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }

        node->next = blockedMemory;
        node->address = code;
        blockedMemory = node;

        code = (unsigned char *)allocMemory(NULL,
            alignedImageSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE,
            userdata);
        if (code == NULL) {
            FreePointerList(blockedMemory, freeMemory, userdata);
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }
#endif

    result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORYMODULE));
    if (result == NULL) {
        freeMemory(code, 0, MEM_RELEASE, userdata);
#ifdef _WIN64
        FreePointerList(blockedMemory, freeMemory, userdata);
#endif
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

	result->heapmodules = HeapCreate(HEAP_ZERO_MEMORY, sizeof(HCUSTOMMODULE), 0); // used for various things
    result->codeBase = code;
    result->isDLL = (old_header->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    result->alloc = allocMemory;
    result->free = freeMemory;
    result->loadLibrary = loadLibrary;
    result->getProcAddress = getProcAddress;
    result->freeLibrary = freeLibrary;
    result->userdata = userdata;
	result->pageSize = old_header->OptionalHeader.SectionAlignment; // sysInfo.dwPageSize;
#ifdef _WIN64
    result->blockedMemory = blockedMemory;
#endif

    if (size && !CheckSize(size, old_header->OptionalHeader.SizeOfHeaders)) {
        goto error;
    }

    // commit memory for headers
    headers = (unsigned char *)allocMemory(code,
        old_header->OptionalHeader.SizeOfHeaders,
        MEM_COMMIT,
        PAGE_READWRITE,
        userdata);

    // copy PE header to code
    memcpy(headers, dos_header, old_header->OptionalHeader.SizeOfHeaders);
    result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

    // update position
    result->headers->OptionalHeader.ImageBase = (uintptr_t)code;

    // copy sections from DLL file block to new memory location
    if (!CopySections((const unsigned char *) data, size, old_header, result)) {
        goto error;
    }

    // adjust base address of imported data
    locationDelta = (ptrdiff_t)(result->headers->OptionalHeader.ImageBase - old_header->OptionalHeader.ImageBase);
    if (locationDelta != 0) {
        result->isRelocated = PerformBaseRelocation(result, locationDelta);
    } else {
        result->isRelocated = TRUE;
    }
	
    // load required dlls and adjust function table of imports
    if (!BuildImportTable(result)) {
        goto error;
    }

    // mark memory pages depending on section headers and release
    // sections that are marked as "discardable"
    if (!FinalizeSections(result)) {
        goto error;
    }

    // TLS callbacks are executed BEFORE the main loading
	if (old_header->OptionalHeader.ImageBase != (ULONGLONG)g_hInstance && !ExecuteTLS(result)) {
        goto error;
    }

    // get entry point of loaded library
    if (result->headers->OptionalHeader.AddressOfEntryPoint != 0) {
        if (result->isDLL) {
			if (callentry)
			{
				DllEntryProc DllEntry = (DllEntryProc)(LPVOID)(code + result->headers->OptionalHeader.AddressOfEntryPoint);

				PCRITICAL_SECTION aLoaderLock; // So no other module can be loaded, expecially due to hooked _RtlPcToFileHeader
#ifdef _M_IX86 // compiles for x86
				aLoaderLock = *(PCRITICAL_SECTION*)(__readfsdword(0x30) + 0xA0); //PEB->LoaderLock
#elif _M_AMD64 // compiles for x64
				aLoaderLock = *(PCRITICAL_SECTION*)(__readgsqword(0x60) + 0x110); //PEB->LoaderLock //0x60 because offset is doubled in 64bit
#endif
				// set start and end of memory for our module so HookRtlPcToFileHeader can report properly
				currentModuleStart = result->codeBase;
				currentModuleEnd = result->codeBase + result->headers->OptionalHeader.SizeOfImage;
				EnterCriticalSection(aLoaderLock);
				pHook = MinHookEnable(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlPcToFileHeader"), &HookRtlPcToFileHeader, &hHeap);
				// notify library about attaching to process
				BOOL successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, result);
				// Disable hook if it was enabled before
				MinHookDisable(pHook);
				HeapFree(hHeap, 0, pHook);
				HeapDestroy(hHeap);
				LeaveCriticalSection(aLoaderLock);

				if (!successfull) {
					SetLastError(ERROR_DLL_INIT_FAILED);
					goto error;
				}
			}
            result->initialized = TRUE;
        } else {
            result->exeEntry = (ExeEntryProc)(LPVOID)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
        }
    } else {
        result->exeEntry = NULL;
    }

    return (HMEMORYMODULE)result;

error:
    // cleanup
    MemoryFreeLibrary(result);
    return NULL;
}

static int _compare(const void *a, const void *b)
{
    const struct ExportNameEntry *p1 = (const struct ExportNameEntry*) a;
    const struct ExportNameEntry *p2 = (const struct ExportNameEntry*) b;
    return strcmp(p1->name, p2->name);
}

static int _find(const void *a, const void *b)
{
    LPCSTR *name = (LPCSTR *) a;
    const struct ExportNameEntry *p = (const struct ExportNameEntry*) b;
    return strcmp(*name, p->name);
}

FARPROC MemoryGetProcAddress(HMEMORYMODULE mod, LPCSTR name)
{
    PMEMORYMODULE module = (PMEMORYMODULE)mod;
    unsigned char *codeBase = module->codeBase;
    DWORD idx = 0;
    PIMAGE_EXPORT_DIRECTORY exports;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_EXPORT);
    if (directory->Size == 0) {
        // no export table found
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    exports = (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0) {
        // DLL doesn't export anything
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    if (HIWORD(name) == 0) {
        // load function by ordinal value
        if (LOWORD(name) < exports->Base) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return NULL;
        }

        idx = LOWORD(name) - exports->Base;
    } else if (!exports->NumberOfNames) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    } else {
        const struct ExportNameEntry *found;

        // Lazily build name table and sort it by names
        if (!module->nameExportsTable) {
            DWORD i;
            DWORD *nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
            WORD *ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);
			struct ExportNameEntry *entry = (struct ExportNameEntry*) HeapAlloc(module->heapmodules, HEAP_ZERO_MEMORY, exports->NumberOfNames * sizeof(struct ExportNameEntry));
            module->nameExportsTable = entry;
            if (!entry) {
                SetLastError(ERROR_OUTOFMEMORY);
                return NULL;
            }
            for (i=0; i<exports->NumberOfNames; i++, nameRef++, ordinal++, entry++) {
                entry->name = (const char *) (codeBase + (*nameRef));
                entry->idx = *ordinal;
            }
            qsort(module->nameExportsTable,
                    exports->NumberOfNames,
                    sizeof(struct ExportNameEntry), _compare);
        }

        // search function name in list of exported names with binary search
        found = (const struct ExportNameEntry*) bsearch(&name,
                module->nameExportsTable,
                exports->NumberOfNames,
                sizeof(struct ExportNameEntry), _find);
        if (!found) {
            // exported symbol not found
            SetLastError(ERROR_PROC_NOT_FOUND);
            return NULL;
        }

        idx = found->idx;
    }

    if (idx > exports->NumberOfFunctions) {
        // name <-> ordinal number don't match
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    // AddressOfFunctions contains the RVAs to the "real" functions
    return (FARPROC)(LPVOID)(codeBase + (*(DWORD *) (codeBase + exports->AddressOfFunctions + (idx*4))));
}

void MemoryFreeLibrary(HMEMORYMODULE mod)
{
    PMEMORYMODULE module = (PMEMORYMODULE)mod;

    if (module == NULL) {
        return;
    }
    if (module->initialized) {
        // notify library about detaching from process
        DllEntryProc DllEntry = (DllEntryProc)(LPVOID)(module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
        (*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
    }

	if (module->nameExportsTable != NULL)
		HeapFree(module->heapmodules, 0, module->nameExportsTable);
	if (module->modules != NULL) {
		// free previously opened libraries
		int i;
		for (i = 0; i < module->numModules; i++) {
			if (module->modules[i] != NULL) {
				module->freeLibrary(module->modules[i], module->userdata);
			}
		}
		HeapFree(module->heapmodules, 0, module->modules);
	}
	HeapDestroy(module->heapmodules);


    if (module->codeBase != NULL) {
        // release memory of library
        module->free(module->codeBase, 0, MEM_RELEASE, module->userdata);
    }

#ifdef _WIN64
    FreePointerList(module->blockedMemory, module->free, module->userdata);
#endif
    HeapFree(GetProcessHeap(), 0, module);
}

HANDLE MemoryCallEntryPoint(HMEMORYMODULE mod, LPTSTR cmdLine)
{
	PMEMORYMODULE module = (PMEMORYMODULE)mod;

	if (module == NULL || module->isDLL || module->exeEntry == NULL || !module->isRelocated) {
		return 0;
	}
	LPTSTR acmd = GetCommandLine();
	LPTSTR bkpcmd = (LPTSTR)_malloca((_tcslen(acmd) + 1) * sizeof(TCHAR));
	_tcscpy(bkpcmd, acmd);
	_tcscpy(acmd, cmdLine);
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void *)) module->exeEntry, NULL, 0, 0);
	WaitForSingleObject(hThread, 100);
	_tcscpy(acmd, bkpcmd);
	_freea(bkpcmd);
	return hThread;
}

#define DEFAULT_LANGUAGE        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

HMEMORYRSRC MemoryFindResource(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type)
{
    return MemoryFindResourceEx(module, name, type, DEFAULT_LANGUAGE);
}

static PIMAGE_RESOURCE_DIRECTORY_ENTRY _MemorySearchResourceEntry(
    void *root,
    PIMAGE_RESOURCE_DIRECTORY resources,
    LPCTSTR key)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entries = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (resources + 1);
    PIMAGE_RESOURCE_DIRECTORY_ENTRY result = NULL;
    DWORD start;
    DWORD end;
    DWORD middle;

    if (!IS_INTRESOURCE(key) && key[0] == TEXT('#')) {
        // special case: resource id given as string
        TCHAR *endpos = NULL;
        long int tmpkey = (WORD) _tcstol((TCHAR *) &key[1], &endpos, 10);
        if (tmpkey <= 0xffff && lstrlen(endpos) == 0) {
            key = MAKEINTRESOURCE(tmpkey);
        }
    }

    // entries are stored as ordered list of named entries,
    // followed by an ordered list of id entries - we can do
    // a binary search to find faster...
    if (IS_INTRESOURCE(key)) {
        WORD check = (WORD) (uintptr_t) key;
        start = resources->NumberOfNamedEntries;
        end = start + resources->NumberOfIdEntries;

        while (end > start) {
            WORD entryName;
            middle = (start + end) >> 1;
            entryName = (WORD) entries[middle].Name;
            if (check < entryName) {
                end = (end != middle ? middle : middle-1);
            } else if (check > entryName) {
                start = (start != middle ? middle : middle+1);
            } else {
                result = &entries[middle];
                break;
            }
        }
    } else {
        LPCWSTR searchKey;
        size_t searchKeyLen = _tcslen(key);
#if defined(UNICODE)
        searchKey = key;
#else
        // Resource names are always stored using 16bit characters, need to
        // convert string we search for.
#define MAX_LOCAL_KEY_LENGTH 2048
        // In most cases resource names are short, so optimize for that by
        // using a pre-allocated array.
        wchar_t _searchKeySpace[MAX_LOCAL_KEY_LENGTH+1];
        LPWSTR _searchKey;
        if (searchKeyLen > MAX_LOCAL_KEY_LENGTH) {
            size_t _searchKeySize = (searchKeyLen + 1) * sizeof(wchar_t);
            _searchKey = (LPWSTR) malloc(_searchKeySize);
            if (_searchKey == NULL) {
                SetLastError(ERROR_OUTOFMEMORY);
                return NULL;
            }
        } else {
            _searchKey = &_searchKeySpace[0];
        }

        mbstowcs(_searchKey, key, searchKeyLen);
        _searchKey[searchKeyLen] = 0;
        searchKey = _searchKey;
#endif
        start = 0;
        end = resources->NumberOfNamedEntries;
        while (end > start) {
            int cmp;
            PIMAGE_RESOURCE_DIR_STRING_U resourceString;
            middle = (start + end) >> 1;
            resourceString = (PIMAGE_RESOURCE_DIR_STRING_U) OffsetPointer(root, entries[middle].Name & 0x7FFFFFFF);
            cmp = _wcsnicmp(searchKey, resourceString->NameString, resourceString->Length);
            if (cmp == 0) {
                // Handle partial match
                if (searchKeyLen > resourceString->Length) {
                    cmp = 1;
                } else if (searchKeyLen < resourceString->Length) {
                    cmp = -1;
                }
            }
            if (cmp < 0) {
                end = (middle != end ? middle : middle-1);
            } else if (cmp > 0) {
                start = (middle != start ? middle : middle+1);
            } else {
                result = &entries[middle];
                break;
            }
        }
#ifndef _UNICODE
        if (searchKeyLen > MAX_LOCAL_KEY_LENGTH) {
            free(_searchKey);
        }
#undef MAX_LOCAL_KEY_LENGTH
#endif
    }

    return result;
}

HMEMORYRSRC MemoryFindResourceEx(HMEMORYMODULE module, LPCTSTR name, LPCTSTR type, WORD language)
{
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE) module, IMAGE_DIRECTORY_ENTRY_RESOURCE);
    PIMAGE_RESOURCE_DIRECTORY rootResources;
    PIMAGE_RESOURCE_DIRECTORY nameResources;
    PIMAGE_RESOURCE_DIRECTORY typeResources;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY foundLanguage;
    if (directory->Size == 0) {
        // no resource table found
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        return NULL;
    }

    if (language == DEFAULT_LANGUAGE) {
        // use language from current thread
        language = LANGIDFROMLCID(GetThreadLocale());
    }

    // resources are stored as three-level tree
    // - first node is the type
    // - second node is the name
    // - third node is the language
    rootResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress);
    foundType = _MemorySearchResourceEntry(rootResources, rootResources, type);
    if (foundType == NULL) {
        SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
        return NULL;
    }

    typeResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress + (foundType->OffsetToData & 0x7fffffff));
    foundName = _MemorySearchResourceEntry(rootResources, typeResources, name);
    if (foundName == NULL) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    nameResources = (PIMAGE_RESOURCE_DIRECTORY) (codeBase + directory->VirtualAddress + (foundName->OffsetToData & 0x7fffffff));
    foundLanguage = _MemorySearchResourceEntry(rootResources, nameResources, (LPCTSTR) (uintptr_t) language);
    if (foundLanguage == NULL) {
        // requested language not found, use first available
        if (nameResources->NumberOfIdEntries == 0) {
            SetLastError(ERROR_RESOURCE_LANG_NOT_FOUND);
            return NULL;
        }

        foundLanguage = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (nameResources + 1);
    }

    return (codeBase + directory->VirtualAddress + (foundLanguage->OffsetToData & 0x7fffffff));
}

DWORD MemorySizeOfResource(HMEMORYMODULE module, HMEMORYRSRC resource)
{
    PIMAGE_RESOURCE_DATA_ENTRY entry;
    UNREFERENCED_PARAMETER(module);
    entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    if (entry == NULL) {
        return 0;
    }

    return entry->Size;
}

LPVOID MemoryLoadResource(HMEMORYMODULE module, HMEMORYRSRC resource)
{
    unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
    PIMAGE_RESOURCE_DATA_ENTRY entry = (PIMAGE_RESOURCE_DATA_ENTRY) resource;
    if (entry == NULL) {
        return NULL;
    }

    return codeBase + entry->OffsetToData;
}

LPWSTR
MemoryLoadString(HMEMORYMODULE module, UINT id)
{
    return MemoryLoadStringEx(module, id, DEFAULT_LANGUAGE);
}

LPWSTR
MemoryLoadStringEx(HMEMORYMODULE module, UINT id, WORD language)
{
    HMEMORYRSRC resource;
    PIMAGE_RESOURCE_DIR_STRING_U data;
    DWORD size;

    resource = MemoryFindResourceEx(module, MAKEINTRESOURCE((id >> 4) + 1), RT_STRING, language);
    if (resource == NULL) {
        return NULL;
    }

    data = (PIMAGE_RESOURCE_DIR_STRING_U) MemoryLoadResource(module, resource);
    id = id & 0x0f;
    while (id--) {
        data = (PIMAGE_RESOURCE_DIR_STRING_U) OffsetPointer(data, (data->Length + 1) * sizeof(WCHAR));
    }
    if (size = data->Length == 0) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    return data->NameString;
}
