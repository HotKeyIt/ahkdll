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
 */

#include "stdafx.h" // pre-compiled headers
#include "MemoryModule.h"
#include "globaldata.h" // for access to many global vars

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
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#if _MSC_VER
// Disable warning about data -> function pointer conversion
#pragma warning(disable:4055)
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
// hook function and global vars for HookRtlPcToFileHeader
typedef PVOID(*MyRtlPcToFileHeader)(PVOID PcValue, PVOID *BaseOfImage);
MyRtlPcToFileHeader _RtlPcToFileHeader = (MyRtlPcToFileHeader)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlPcToFileHeader");

HMEMORYMODULE currentModuleStart;
PVOID currentModuleEnd;

typedef struct {
    LPVOID address;
    LPVOID alignedAddress;
    DWORD size;
    DWORD characteristics;
    BOOL last;
} SECTIONFINALIZEDATA, *PSECTIONFINALIZEDATA;

#define GET_HEADER_DICTIONARY(module, idx)  &(module)->headers->OptionalHeader.DataDirectory[idx]
#define ALIGN_DOWN(address, alignment)      (LPVOID)((uintptr_t)(address) & ~((alignment) - 1))
#define ALIGN_VALUE_UP(value, alignment)    (((value) + (alignment) - 1) & ~((alignment) - 1))

PVOID WINAPI HookRtlPcToFileHeader(PVOID PcValue, PVOID *BaseOfImage)
{
	if (PcValue >= currentModuleStart && PcValue < currentModuleEnd)
		return *BaseOfImage = currentModuleStart;
	else
		return _RtlPcToFileHeader(PcValue, BaseOfImage);
}

#ifdef DEBUG_OUTPUT
static void
OutputLastError(const char *msg)
{
    LPVOID tmp;
    char *tmpmsg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
    tmpmsg = (char *)LocalAlloc(LPTR, strlen(msg) + strlen((const char*)tmp) + 3);
    sprintf(tmpmsg, "%s: %s", msg, tmp);
    OutputDebugStringA(tmpmsg);
    LocalFree(tmpmsg);
    LocalFree(tmp);
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
                dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
                    section_size,
                    MEM_COMMIT,
                    PAGE_EXECUTE_READWRITE);
                if (dest == NULL) {
                    return FALSE;
                }

                // Always use position from file to support alignments smaller
                // than page size.
                dest = codeBase + section->VirtualAddress;
                section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;
                memset(dest, 0, section_size);
            }

            // section is empty
            continue;
        }

        if (size && !CheckSize(size, section->PointerToRawData + section->SizeOfRawData)) {
            return FALSE;
        }

        // commit memory block and copy data from dll
        dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
                            section->SizeOfRawData,
                            MEM_COMMIT,
                            PAGE_EXECUTE_READWRITE);
        if (dest == NULL) {
            return FALSE;
        }

        // Always use position from file to support alignments smaller
        // than page size.
        dest = codeBase + section->VirtualAddress;
        memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
		section->Misc.PhysicalAddress = ((DWORD)(((POINTER_TYPE)(dest)) & 0xffffffff));
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

static DWORD
GetRealSectionSize(PMEMORYMODULE module, PIMAGE_SECTION_HEADER section) {
    DWORD size = section->SizeOfRawData;
    if (size == 0) {
        if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfInitializedData;
        } else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            size = module->headers->OptionalHeader.SizeOfUninitializedData;
        }
    }
    return size;
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
            VirtualFree(sectionData->address, sectionData->size, MEM_DECOMMIT);
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
#ifdef DEBUG_OUTPUT
		OutputLastError("Error protecting memory page");
#endif
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
    uintptr_t imageOffset = (module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
    #define imageOffset 0
#endif
    SECTIONFINALIZEDATA sectionData;
    sectionData.address = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
    sectionData.alignedAddress = ALIGN_DOWN(sectionData.address, module->pageSize);
    sectionData.size = GetRealSectionSize(module, section);
    sectionData.characteristics = section->Characteristics;
    sectionData.last = FALSE;
    section++;

    // loop through all sections and change access flags
    for (i=1; i<module->headers->FileHeader.NumberOfSections; i++, section++) {
        LPVOID sectionAddress = (LPVOID)((uintptr_t)section->Misc.PhysicalAddress | imageOffset);
        LPVOID alignedAddress = ALIGN_DOWN(sectionAddress, module->pageSize);
        DWORD sectionSize = GetRealSectionSize(module, section);
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
            sectionData.size = (DWORD)((((uintptr_t)sectionAddress) + sectionSize) - (uintptr_t) sectionData.address);
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
#ifndef _WIN64
#undef imageOffset
#endif
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
        unsigned short *relInfo = (unsigned short *)((unsigned char *)relocation + IMAGE_SIZEOF_BASE_RELOCATION);
        for (i=0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++) {
            DWORD *patchAddrHL;
#ifdef _WIN64
            ULONGLONG *patchAddr64;
#endif
            int type, offset;

            // the upper 4 bits define the type of relocation
            type = *relInfo >> 12;
            // the lower 12 bits define the offset
            offset = *relInfo & 0xfff;

            switch (type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                // skip relocation
                break;

            case IMAGE_REL_BASED_HIGHLOW:
                // change complete 32 bit address
                patchAddrHL = (DWORD *) (dest + offset);
                *patchAddrHL += (DWORD) delta;
                break;

#ifdef _WIN64
            case IMAGE_REL_BASED_DIR64:
                patchAddr64 = (ULONGLONG *) (dest + offset);
                *patchAddr64 += (ULONGLONG) delta;
                break;
#endif

            default:
                //printf("Unknown relocation: %d\n", type);
                break;
            }
        }

        // advance to next relocation block
        relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
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
    if (directory->Size == 0){
        return TRUE;
    }
    
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);
    // Following will be used to resolve manifest in module
    if (resource->Size)
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
        HCUSTOMMODULE *tmp;
        HCUSTOMMODULE handle = NULL;
        char *isMsvcr = NULL;
        if (g_hMSVCR != NULL && (isMsvcr = strstr((LPSTR)(codeBase + importDesc->Name), "MSVCR100.dll")))
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
            handle = module->loadLibrary((LPCSTR) (codeBase + importDesc->Name), module->userdata);

        if (handle == NULL) {
            SetLastError(ERROR_MOD_NOT_FOUND);
            result = FALSE;
            break;
        }
        if (!isMsvcr)
        {
            tmp = (HCUSTOMMODULE *)realloc(module->modules, (module->numModules + 1)*(sizeof(HCUSTOMMODULE)));
            if (tmp == NULL) {
                module->freeLibrary(handle, module->userdata);
                SetLastError(ERROR_OUTOFMEMORY);
                result = 0;
                break;
            }
            module->modules = tmp;
            if (module->numModules == 1)
                module->modules[0] = NULL;
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

HMEMORYMODULE MemoryLoadLibrary(const void *data, size_t size)
{
    return MemoryLoadLibraryEx(data, size, MemoryDefaultLoadLibrary, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL);
}

HMEMORYMODULE MemoryLoadLibraryEx(const void *data, size_t size,
    CustomLoadLibraryFunc loadLibrary,
    CustomGetProcAddressFunc getProcAddress,
    CustomFreeLibraryFunc freeLibrary,
    void *userdata)
{
    PMEMORYMODULE result = NULL;
    PIMAGE_DOS_HEADER dos_header;
    PIMAGE_NT_HEADERS old_header;
    unsigned char *code, *headers;
    ptrdiff_t locationDelta;
    SYSTEM_INFO sysInfo;
    PIMAGE_SECTION_HEADER section;
    DWORD i;
    size_t optionalSectionSize;
    size_t lastSectionEnd = 0;
    size_t alignedImageSize;

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

#ifdef _WIN64
    if (old_header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
#else
    if (old_header->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
#endif
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

    GetNativeSystemInfo(&sysInfo);
    alignedImageSize = ALIGN_VALUE_UP(old_header->OptionalHeader.SizeOfImage, sysInfo.dwPageSize);
    if (alignedImageSize < ALIGN_VALUE_UP(lastSectionEnd, sysInfo.dwPageSize)) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }

    // reserve memory for image of library
    // XXX: is it correct to commit the complete memory region at once?
    //      calling DllEntry raises an exception if we don't...
    code = (unsigned char *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
        alignedImageSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    if (code == NULL) {
        // try to allocate memory at arbitrary position
        code = (unsigned char *)VirtualAlloc(NULL,
            alignedImageSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);
        if (code == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
    }

    result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORYMODULE));
    if (result == NULL) {
        VirtualFree(code, 0, MEM_RELEASE);
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    result->codeBase = code;
    result->isDLL = (old_header->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    result->loadLibrary = loadLibrary;
    result->getProcAddress = getProcAddress;
    result->freeLibrary = freeLibrary;
    result->userdata = userdata;
    result->pageSize = sysInfo.dwPageSize;

    if (size && !CheckSize(size, old_header->OptionalHeader.SizeOfHeaders)) {
        goto error;
    }

    // commit memory for headers
    headers = (unsigned char *)VirtualAlloc(code,
        old_header->OptionalHeader.SizeOfHeaders,
        MEM_COMMIT,
        PAGE_READWRITE);

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
	if (old_header->OptionalHeader.ImageBase != (DWORD)g_hInstance && !ExecuteTLS(result)) {
        goto error;
    }

    // get entry point of loaded library
    if (result->headers->OptionalHeader.AddressOfEntryPoint != 0) {
        if (result->isDLL) {
            DllEntryProc DllEntry = (DllEntryProc)(LPVOID)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
            
            PCRITICAL_SECTION aLoaderLock; // So no other module can be loaded, expecially due to hooked _RtlPcToFileHeader
#ifdef _M_IX86 // compiles for x86
            aLoaderLock = *(PCRITICAL_SECTION*)(__readfsdword(0x30) + 0xA0); //PEB->LoaderLock
#elif _M_AMD64 // compiles for x64
            aLoaderLock = *(PCRITICAL_SECTION*)(__readgsqword(0x60) + 0x110); //PEB->LoaderLock //0x60 because offset is doubled in 64bit
#endif
            HANDLE hHeap = NULL;
            // set start and end of memory for our module so HookRtlPcToFileHeader can report properly
            currentModuleStart = result->codeBase;
            currentModuleEnd = result->codeBase + result->headers->OptionalHeader.SizeOfImage;
            if (!_RtlPcToFileHeader)
                _RtlPcToFileHeader = (MyRtlPcToFileHeader)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlPcToFileHeader");
            EnterCriticalSection(aLoaderLock);
            PHOOK_ENTRY pHook = MinHookEnable(_RtlPcToFileHeader, &HookRtlPcToFileHeader, &hHeap);
            // notify library about attaching to process
            BOOL successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, result);
            // Disable hook if it was enabled before
            if (pHook)
                MinHookDisable(pHook);
            if (hHeap)
                HeapDestroy(hHeap);
            LeaveCriticalSection(aLoaderLock);

            if (!successfull) {
                SetLastError(ERROR_DLL_INIT_FAILED);
                goto error;
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

FARPROC MemoryGetProcAddress(HMEMORYMODULE module, LPCSTR name)
{
    unsigned char *codeBase = ((PMEMORYMODULE)module)->codeBase;
    DWORD idx = 0;
    PIMAGE_EXPORT_DIRECTORY exports;
    PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE)module, IMAGE_DIRECTORY_ENTRY_EXPORT);
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
    } else {
        // search function name in list of exported names
        DWORD i;
        DWORD *nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
        WORD *ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);
        BOOL found = FALSE;
        for (i=0; i<exports->NumberOfNames; i++, nameRef++, ordinal++) {
            if (_stricmp(name, (const char *) (codeBase + (*nameRef))) == 0) {
                idx = *ordinal;
                found = TRUE;
                break;
            }
        }

        if (!found) {
            // exported symbol not found
            SetLastError(ERROR_PROC_NOT_FOUND);
            return NULL;
        }
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

    if (module->modules != NULL) {
        // free previously opened libraries
        int i;
        for (i=0; i<module->numModules; i++) {
            if (module->modules[i] != NULL) {
                module->freeLibrary(module->modules[i], module->userdata);
            }
        }

        free(module->modules);
    }

    if (module->codeBase != NULL) {
        // release memory of library
        VirtualFree(module->codeBase, 0, MEM_RELEASE);
    }

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
            resourceString = (PIMAGE_RESOURCE_DIR_STRING_U) (((char *) root) + (entries[middle].Name & 0x7FFFFFFF));
            cmp = _wcsnicmp(searchKey, resourceString->NameString, resourceString->Length);
            if (cmp == 0) {
                // Handle partial match
                cmp = (int)(searchKeyLen - resourceString->Length);
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

LPVOID MemoryLoadString(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize)
{
    return MemoryLoadStringEx(module, id, buffer, maxsize, DEFAULT_LANGUAGE);
}

LPVOID MemoryLoadStringEx(HMEMORYMODULE module, UINT id, LPTSTR buffer, int maxsize, WORD language)
{
    HMEMORYRSRC resource;
    PIMAGE_RESOURCE_DIR_STRING_U data;
    DWORD size;
    if (buffer && maxsize == 0) {
        return 0;
    }

    resource = MemoryFindResourceEx(module, MAKEINTRESOURCE((id >> 4) + 1), RT_STRING, language);
    if (resource == NULL) {
        buffer[0] = 0;
        return 0;
    }

    data = (PIMAGE_RESOURCE_DIR_STRING_U) MemoryLoadResource(module, resource);
    id = id & 0x0f;
    while (id--) {
        data = (PIMAGE_RESOURCE_DIR_STRING_U) (((char *) data) + (data->Length + 1) * sizeof(WCHAR));
    }
    if (data->Length == 0) {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        buffer[0] = 0;
        return 0;
    } else if (!buffer)
        return data->NameString;

    size = data->Length;
    if (size >= (DWORD) maxsize) {
        size = maxsize;
    } else {
        buffer[size] = 0;
    }
#if defined(UNICODE)
    wcsncpy(buffer, data->NameString, size);
#else
    wcstombs(buffer, data->NameString, size);
#endif
    return (LPVOID)size;
}
