/*
 * Undocumented Windows structures
 *
 * Found on http://undocumented.ntinternals.net/
 */

#ifndef __NT_INTERNALS
#define __NT_INTERNALS

#include <windows.h>
#include <winternl.h>


// constants from the article
// "What Goes On Inside Windows 2000: Solving the Mysteries of the Loader"
// by Russ Osterlund
// http://msdn.microsoft.com/msdnmag/issues/02/03/Loader/default.aspx
#define MAX_DLL_NAME_LENGTH	0x214

#define STATIC_LINK				0x00000002
#define IMAGE_DLL				0x00000004
#define LOAD_IN_PROGRESS		0x00001000
#define UNLOAD_IN_PROGRESS		0x00002000
#define ENTRY_PROCESSED			0x00004000
#define ENTRY_INSERTED			0x00008000
#define CURRENT_LOAD			0x00010000
#define FAILED_BUILTIN_LOAD		0x00020000
#define DONT_CALL_FOR_THREAD	0x00040000
#define PROCESS_ATTACH_CALLED	0x00080000
#define DEBUG_SYMBOLS_LOADED	0x00100000
#define IMAGE_NOT_AT_BASE		0x00200000
#define WX86_IGNORE_MACHINETYPE	0x00400000

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _MMLDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} MMLDR_MODULE, * MMPLDR_MODULE;

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _MMPEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} MMPEB_LDR_DATA, * MMPPEB_LDR_DATA;

/*
 * Documented by:
 *   Reactos
 */
typedef struct _MMRTL_DRIVE_LETTER_CURDIR {
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	UNICODE_STRING DosPath;
} MMRTL_DRIVE_LETTER_CURDIR, * MMPRTL_DRIVE_LETTER_CURDIR;

/*
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _MMRTL_USER_PROCESS_PARAMETERS {
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
	UNICODE_STRING CurrentDirectoryPath;
	HANDLE CurrentDirectoryHandle;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingPositionLeft;
	ULONG StartingPositionTop;
	ULONG Width;
	ULONG Height;
	ULONG CharWidth;
	ULONG CharHeight;
	ULONG ConsoleTextAttributes;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	MMRTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} MMRTL_USER_PROCESS_PARAMETERS, * MMPRTL_USER_PROCESS_PARAMETERS;

/*
 * Address of fast-locking routine for PEB
 */
typedef void (*MMPPEBLOCKROUTINE)(
	PVOID PebLock
	);

typedef LPVOID* PPVOID;

/*
 * Structure PEB_FREE_BLOCK is used internally in PEB (Process Enviroment Block)
 * structure for describe free blocks in memory allocated for PEB.
 *
 * Documented by:
 *   Reactos
 */
typedef struct _MMPEB_FREE_BLOCK {
	struct _MMPEB_FREE_BLOCK* Next;
	ULONG Size;
} MMPEB_FREE_BLOCK, * MMPPEB_FREE_BLOCK;

/*
 * Structure PEB (Process Enviroment Block) contains all User-Mode parameters
 * associated by system with current process.
 *
 * Documented by:
 *   Reactos
 *   Tomasz Nowak
 */
typedef struct _MMPEB {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN Spare;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	MMPPEB_LDR_DATA LoaderData;
	MMPRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PVOID FastPebLock;
	MMPPEBLOCKROUTINE FastPebLockRoutine;
	MMPPEBLOCKROUTINE FastPebUnlockRoutine;
	ULONG EnvironmentUpdateCount;
	PPVOID KernelCallbackTable;
	PVOID EventLogSection;
	PVOID EventLog;
	MMPPEB_FREE_BLOCK FreeList;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[0x2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID ReadOnlySharedMemoryHeap;
	PPVOID ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	BYTE Spare2[0x4];
	LARGE_INTEGER CriticalSectionTimeout;
	ULONG HeapSegmentReserve;
	ULONG HeapSegmentCommit;
	ULONG HeapDeCommitTotalFreeThreshold;
	ULONG HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PPVOID* ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	PVOID GdiDCAttributeList;
	PVOID LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	ULONG OSBuildNumber;
	ULONG OSPlatformId;
	ULONG ImageSubSystem;
	ULONG ImageSubSystemMajorVersion;
	ULONG ImageSubSystemMinorVersion;
	ULONG GdiHandleBuffer[0x22];
	ULONG PostProcessInitRoutine;
	ULONG TlsExpansionBitmap;
	BYTE TlsExpansionBitmapBits[0x80];
	ULONG SessionId;
} MMPEB, * MMPPEB;

#endif  // __NT_INTERNALS