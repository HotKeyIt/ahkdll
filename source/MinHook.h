/*
*  MinHook - The Minimalistic API Hooking Library for x64/x86
*  Copyright (C) 2009-2014 Tsuda Kageyu.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   1. Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
*  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
*  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
*  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Trampoline
#pragma once
#pragma pack(push, 1)

// Structs for writing x86/x64 instructions.

// 8-bit relative jump.
typedef struct _JMP_REL_SHORT
{
	UINT8  opcode;      // EB xx: JMP +2+xx
	UINT8  operand;
} JMP_REL_SHORT, *PJMP_REL_SHORT;

// 32-bit direct relative jump/call.
typedef struct _JMP_REL
{
	UINT8  opcode;      // E9/E8 xxxxxxxx: JMP/CALL +5+xxxxxxxx
	UINT32 operand;     // Relative destination address
} JMP_REL, *PJMP_REL, CALL_REL;

// 64-bit indirect absolute jump.
typedef struct _JMP_ABS
{
	UINT8  opcode0;     // FF25 00000000: JMP [+6]
	UINT8  opcode1;
	UINT32 dummy;
	UINT64 address;     // Absolute destination address
} JMP_ABS, *PJMP_ABS;

// 64-bit indirect absolute call.
typedef struct _CALL_ABS
{
	UINT8  opcode0;     // FF15 00000002: CALL [+6]
	UINT8  opcode1;
	UINT32 dummy0;
	UINT8  dummy1;      // EB 08:         JMP +10
	UINT8  dummy2;
	UINT64 address;     // Absolute destination address
} CALL_ABS;

// 32-bit direct relative conditional jumps.
typedef struct _JCC_REL
{
	UINT8  opcode0;     // 0F8* xxxxxxxx: J** +6+xxxxxxxx
	UINT8  opcode1;
	UINT32 operand;     // Relative destination address
} JCC_REL;

// 64bit indirect absolute conditional jumps that x64 lacks.
typedef struct _JCC_ABS
{
	UINT8  opcode;      // 7* 0E:         J** +16
	UINT8  dummy0;
	UINT8  dummy1;      // FF25 00000000: JMP [+6]
	UINT8  dummy2;
	UINT32 dummy3;
	UINT64 address;     // Absolute destination address
} JCC_ABS;

#pragma pack(pop)

typedef struct _TRAMPOLINE
{
	LPVOID pTarget;         // [In] Address of the target function.
	LPVOID pDetour;         // [In] Address of the detour function.
	LPVOID pTrampoline;     // [In] Buffer address for the trampoline and relay function.

#ifdef _M_X64
	LPVOID pRelay;          // [Out] Address of the relay function.
#endif
	BOOL   patchAbove;      // [Out] Should use the hot patch area?
	UINT   nIP;             // [Out] Number of the instruction boundaries.
	UINT8  oldIPs[8];       // [Out] Instruction boundaries of the target function.
	UINT8  newIPs[8];       // [Out] Instruction boundaries of the trampoline function.
} TRAMPOLINE, *PTRAMPOLINE;

#ifdef _M_X64
/*
* Hacker Disassembler Engine 64
* Copyright (c) 2008-2009, Vyacheslav Patkov.
* All rights reserved.
*
* hde64.h: C/C++ header file
*
*/

#ifndef _HDE64_H_
#define _HDE64_H_

/* stdint.h - C99 standard header
* http://en.wikipedia.org/wiki/stdint.h
*
* if your compiler doesn't contain "stdint.h" header (for
* example, Microsoft Visual C++), you can download file:
*   http://www.azillionmonkeys.com/qed/pstdint.h
* and change next line to:
*   #include "pstdint.h"
*/

// Integer types for HDE.
typedef INT8   int8_t;
typedef INT16  int16_t;
typedef INT32  int32_t;
typedef INT64  int64_t;
typedef UINT8  uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_IMM64         0x00000020
#define F_DISP8         0x00000040
#define F_DISP16        0x00000080
#define F_DISP32        0x00000100
#define F_RELATIVE      0x00000200
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_REX    0x40000000
#define F_PREFIX_ANY    0x7f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#pragma pack(push,1)

typedef struct {
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t rex;
	uint8_t rex_w;
	uint8_t rex_r;
	uint8_t rex_x;
	uint8_t rex_b;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union {
		uint8_t imm8;
		uint16_t imm16;
		uint32_t imm32;
		uint64_t imm64;
	} imm;
	union {
		uint8_t disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde64s;

/*
 * Hacker Disassembler Engine 64 C
 * Copyright (c) 2008-2009, Vyacheslav Patkov.
 * All rights reserved.
 *
 */

#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xfd
#define DELTA_FPU_MODRM    0x104
#define DELTA_PREFIXES     0x13c
#define DELTA_OP_LOCK_OK   0x1ae
#define DELTA_OP2_LOCK_OK  0x1c6
#define DELTA_OP_ONLY_MEM  0x1d8
#define DELTA_OP2_ONLY_MEM 0x1e7


#ifdef __cplusplus
extern "C" {
#endif

	/* __cdecl */

	unsigned int hde64_disasm(const void* code, hde64s* hs);

#ifdef __cplusplus
}
#endif

#endif /* _HDE64_H_ */
typedef hde64s HDE;
#define HDE_DISASM(code, hs) hde64_disasm(code, hs)
#else
/*
* Hacker Disassembler Engine 32
* Copyright (c) 2006-2009, Vyacheslav Patkov.
* All rights reserved.
*
* hde32.h: C/C++ header file
*
*/

#ifndef _HDE32_H_
#define _HDE32_H_

/* stdint.h - C99 standard header
* http://en.wikipedia.org/wiki/stdint.h
*
* if your compiler doesn't contain "stdint.h" header (for
* example, Microsoft Visual C++), you can download file:
*   http://www.azillionmonkeys.com/qed/pstdint.h
* and change next line to:
*   #include "pstdint.h"
*/
// Integer types for HDE.
typedef INT8   int8_t;
typedef INT16  int16_t;
typedef INT32  int32_t;
typedef INT64  int64_t;
typedef UINT8  uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_DISP8         0x00000020
#define F_DISP16        0x00000040
#define F_DISP32        0x00000080
#define F_RELATIVE      0x00000100
#define F_2IMM16        0x00000800
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_ANY    0x3f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xf1
#define DELTA_FPU_MODRM    0xf8
#define DELTA_PREFIXES     0x130
#define DELTA_OP_LOCK_OK   0x1a1
#define DELTA_OP2_LOCK_OK  0x1b9
#define DELTA_OP_ONLY_MEM  0x1cb
#define DELTA_OP2_ONLY_MEM 0x1da

#pragma pack(push,1)

typedef struct {
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union {
		uint8_t imm8;
		uint16_t imm16;
		uint32_t imm32;
	} imm;
	union {
		uint8_t disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde32s;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

	/* __cdecl */
	unsigned int hde32_disasm(const void *code, hde32s *hs);

#ifdef __cplusplus
}
#endif

#endif /* _HDE32_H_ */
typedef hde32s HDE;
#define HDE_DISASM(code, hs) hde32_disasm(code, hs)
#endif


// Maximum size of a trampoline function.
#ifdef _M_X64
#define TRAMPOLINE_MAX_SIZE (MEMORY_SLOT_SIZE - sizeof(JMP_ABS))
#else
#define TRAMPOLINE_MAX_SIZE MEMORY_SLOT_SIZE
#endif


// Size of each memory slot.
#ifdef _M_X64
#define MEMORY_SLOT_SIZE 64
#else
#define MEMORY_SLOT_SIZE 32
#endif
// Memory slot.
typedef struct _MEMORY_SLOT
{
	union
	{
		struct _MEMORY_SLOT *pNext;
		UINT8 buffer[MEMORY_SLOT_SIZE];
	};
} MEMORY_SLOT, *PMEMORY_SLOT;

// Memory block info. Placed at the head of each block.
typedef struct _MEMORY_BLOCK
{
	struct _MEMORY_BLOCK *pNext;
	PMEMORY_SLOT pFree;         // First element of the free slot list.
	UINT usedCount;
} MEMORY_BLOCK, *PMEMORY_BLOCK;

// Size of each memory block. (= page size of VirtualAlloc)
#define MEMORY_BLOCK_SIZE 0x1000

// Max range for seeking a memory block. (= 1024MB)
#define MAX_MEMORY_RANGE 0x40000000

// Memory protection flags to check the executable address.
#define PAGE_EXECUTE_FLAGS \
    (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

// Hook information.
typedef struct _HOOK_ENTRY
{
	LPVOID pTarget;             // Address of the target function.
	LPVOID pDetour;             // Address of the detour or relay function.
	LPVOID pTrampoline;         // Address of the trampoline function.
	UINT8  backup[8];           // Original prologue of the target function.

	BOOL   patchAbove : 1;     // Uses the hot patch area.
	BOOL   isEnabled : 1;     // Enabled.
	BOOL   queueEnable : 1;     // Queued for enabling/disabling when != isEnabled.

	UINT   nIP : 4;             // Count of the instruction boundaries.
	UINT8  oldIPs[8];           // Instruction boundaries of the target function.
	UINT8  newIPs[8];           // Instruction boundaries of the trampoline function.
} HOOK_ENTRY, *PHOOK_ENTRY;
#ifdef __cplusplus
extern "C" {
#endif
BOOL CreateTrampolineFunction(PTRAMPOLINE ct);
VOID   InitializeBuffer(VOID);
VOID   UninitializeBuffer(VOID);
LPVOID AllocateBuffer(LPVOID pOrigin);
VOID   FreeBuffer(LPVOID pBuffer);
BOOL   IsExecutableAddress(LPVOID pAddress);
EXPORT PHOOK_ENTRY   MinHookEnable(LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal = NULL);
EXPORT BOOL   MinHookDisable(PHOOK_ENTRY pHook);
#ifdef __cplusplus
}
#endif
