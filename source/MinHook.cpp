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

#include "stdafx.h" // pre-compiled headers
#include <Windows.h>
#include <TlHelp32.h>
#include <intrin.h>

#include "MinHook.h"


//-------------------------------------------------------------------------
// Global Variables:
//-------------------------------------------------------------------------

// First element of the memory block list.
PMEMORY_BLOCK g_pMemoryBlocks;

//-------------------------------------------------------------------------
VOID InitializeBuffer(VOID)
{
	// Nothing to do for now.
}

//-------------------------------------------------------------------------
VOID UninitializeBuffer(VOID)
{
	PMEMORY_BLOCK pBlock = g_pMemoryBlocks;
	g_pMemoryBlocks = NULL;

	while (pBlock)
	{
		PMEMORY_BLOCK pNext = pBlock->pNext;
		VirtualFree(pBlock, 0, MEM_RELEASE);
		pBlock = pNext;
	}
}

//-------------------------------------------------------------------------
static PMEMORY_BLOCK GetMemoryBlock(LPVOID pOrigin)
{
	ULONG_PTR minAddr;
	ULONG_PTR maxAddr;
	PMEMORY_BLOCK pBlock;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (ULONG_PTR)si.lpMinimumApplicationAddress;
	maxAddr = (ULONG_PTR)si.lpMaximumApplicationAddress;

#ifdef _M_X64
	// pOrigin ± 16MB
	if ((ULONG_PTR)pOrigin > MAX_MEMORY_RANGE)
		minAddr = max(minAddr, (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE);

	maxAddr = min(maxAddr, (ULONG_PTR)pOrigin + MAX_MEMORY_RANGE);
#endif

	// Look the registered blocks for a reachable one.
	for (pBlock = g_pMemoryBlocks; pBlock != NULL; pBlock = pBlock->pNext)
	{
#ifdef _M_X64
		// Ignore the blocks too far.
		if ((ULONG_PTR)pBlock < minAddr || (ULONG_PTR)pBlock >= maxAddr)
			continue;
#endif
		// The block has at least one unused slot.
		if (pBlock->pFree != NULL)
			return pBlock;
	}

	// Alloc a new block if not found.
	{
		ULONG_PTR pStart = ((ULONG_PTR)pOrigin / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;
		ULONG_PTR pAlloc;
		for (pAlloc = pStart - MEMORY_BLOCK_SIZE; pAlloc >= minAddr; pAlloc -= MEMORY_BLOCK_SIZE)
		{
			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				(LPVOID)pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
		if (pBlock == NULL)
		{
			for (pAlloc = pStart + MEMORY_BLOCK_SIZE; pAlloc < maxAddr; pAlloc += MEMORY_BLOCK_SIZE)
			{
				pBlock = (PMEMORY_BLOCK)VirtualAlloc(
					(LPVOID)pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (pBlock != NULL)
					break;
			}
		}
	}

	if (pBlock != NULL)
	{
		// Build a linked list of all the slots.
		PMEMORY_SLOT pSlot = (PMEMORY_SLOT)pBlock + 1;
		pBlock->pFree = NULL;
		pBlock->usedCount = 0;
		do
		{
			pSlot->pNext = pBlock->pFree;
			pBlock->pFree = pSlot;
			pSlot++;
		} while ((ULONG_PTR)pSlot - (ULONG_PTR)pBlock <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

		pBlock->pNext = g_pMemoryBlocks;
		g_pMemoryBlocks = pBlock;
	}

	return pBlock;
}

//-------------------------------------------------------------------------
LPVOID AllocateBuffer(LPVOID pOrigin)
{
	PMEMORY_SLOT  pSlot;
	PMEMORY_BLOCK pBlock = GetMemoryBlock(pOrigin);
	if (pBlock == NULL)
		return NULL;

	// Remove an unused slot from the list.
	pSlot = pBlock->pFree;
	pBlock->pFree = pSlot->pNext;
	pBlock->usedCount++;
#ifdef _DEBUG
	// Fill the slot with INT3 for debugging.
	memset(pSlot, 0xCC, sizeof(MEMORY_SLOT));
#endif
	return pSlot;
}

//-------------------------------------------------------------------------
VOID FreeBuffer(LPVOID pBuffer)
{
	PMEMORY_BLOCK pBlock = g_pMemoryBlocks;
	PMEMORY_BLOCK pPrev = NULL;
	ULONG_PTR pTargetBlock = ((ULONG_PTR)pBuffer / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;

	while (pBlock != NULL)
	{
		if ((ULONG_PTR)pBlock == pTargetBlock)
		{
			PMEMORY_SLOT pSlot = (PMEMORY_SLOT)pBuffer;
#ifdef _DEBUG
			// Clear the released slot for debugging.
			memset(pSlot, 0x00, sizeof(MEMORY_SLOT));
#endif
			// Restore the released slot to the list.
			pSlot->pNext = pBlock->pFree;
			pBlock->pFree = pSlot;
			pBlock->usedCount--;

			// Free if unused.
			if (pBlock->usedCount == 0)
			{
				if (pPrev)
					pPrev->pNext = pBlock->pNext;
				else
					g_pMemoryBlocks = pBlock->pNext;

				VirtualFree(pBlock, 0, MEM_RELEASE);
			}

			break;
		}

		pPrev = pBlock;
		pBlock = pBlock->pNext;
	}
}

//-------------------------------------------------------------------------
BOOL IsExecutableAddress(LPVOID pAddress)
{
	MEMORY_BASIC_INFORMATION mi;
	VirtualQuery(pAddress, &mi, sizeof(MEMORY_BASIC_INFORMATION));

	return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}

PHOOK_ENTRY MinHookEnable(LPVOID pTarget, LPVOID pDetour, HANDLE *hHeap)
{
	LPVOID      pBuffer;
	TRAMPOLINE  ct;
	PHOOK_ENTRY pHook;
	DWORD  oldProtect;
	SIZE_T patchSize = sizeof(JMP_REL);
	LPBYTE pPatchTarget;
	PJMP_REL pJmp;

	pBuffer = AllocateBuffer(pTarget);
	if (pBuffer == NULL)
		return NULL;

	ct.pTarget = pTarget;
	ct.pDetour = pDetour;
	ct.pTrampoline = pBuffer;
	if (!CreateTrampolineFunction(&ct))
	{
		FreeBuffer(pBuffer);
		return NULL;
	}
	*hHeap = HeapCreate(0, 0, 0);
	if (*hHeap == NULL)
		return NULL;
	pHook = (PHOOK_ENTRY)HeapAlloc(*hHeap, 0, sizeof(HOOK_ENTRY));
	if (pHook == NULL)
	{
		FreeBuffer(pBuffer);
		return NULL;
	}

	pHook->pTarget = ct.pTarget;
#ifdef _M_X64
	pHook->pDetour = ct.pRelay;
#else
	pHook->pDetour = ct.pDetour;
#endif
	pHook->pTrampoline = ct.pTrampoline;
	pHook->patchAbove = ct.patchAbove;
	pHook->isEnabled = FALSE;
	pHook->queueEnable = FALSE;
	pHook->nIP = ct.nIP;
	memcpy(pHook->oldIPs, ct.oldIPs, ARRAYSIZE(ct.oldIPs));
	memcpy(pHook->newIPs, ct.newIPs, ARRAYSIZE(ct.newIPs));

	// Back up the target function.
	if (ct.patchAbove)
	{
		memcpy(
			pHook->backup,
			(LPBYTE)pTarget - sizeof(JMP_REL),
			sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
	}
	else
	{
		memcpy(pHook->backup, (LPVOID)pTarget, sizeof(JMP_REL));
	}

	// Enable Hook
	pPatchTarget = (LPBYTE)pHook->pTarget;

	if (pHook->patchAbove)
	{
		pPatchTarget -= sizeof(JMP_REL);
		patchSize += sizeof(JMP_REL_SHORT);
	}

	if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		return NULL;

	pJmp = (PJMP_REL)pPatchTarget;
	pJmp->opcode = 0xE9;
	pJmp->operand = (UINT32)((LPBYTE)pHook->pDetour - (pPatchTarget + sizeof(JMP_REL)));
	if (pHook->patchAbove)
	{
		PJMP_REL_SHORT pShortJmp = (PJMP_REL_SHORT)pHook->pTarget;
		pShortJmp->opcode = 0xEB;
		pShortJmp->operand = (UINT8)(0 - (sizeof(JMP_REL_SHORT) + sizeof(JMP_REL)));
	}

	VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);
	return pHook;
}

BOOL MinHookDisable(PHOOK_ENTRY pHook)
{
	DWORD  oldProtect;
	SIZE_T patchSize = sizeof(JMP_REL);
	LPBYTE pPatchTarget = (LPBYTE)pHook->pTarget;

	if (pHook->patchAbove)
	{
		pPatchTarget -= sizeof(JMP_REL);
		patchSize += sizeof(JMP_REL_SHORT);
	}

	if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		return FALSE;
	if (pHook->patchAbove)
		memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
	else
		memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL));
	VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);
	return TRUE;
}
//-------------------------------------------------------------------------
static BOOL IsCodePadding(LPBYTE pInst, UINT size)
{
	UINT i;

	if (pInst[0] != 0x00 && pInst[0] != 0x90 && pInst[0] != 0xCC)
		return FALSE;

	for (i = 1; i < size; ++i)
	{
		if (pInst[i] != pInst[0])
			return FALSE;
	}
	return TRUE;
}

//-------------------------------------------------------------------------
BOOL CreateTrampolineFunction(PTRAMPOLINE ct)
{
#ifdef _M_X64
	CALL_ABS call = {
		0xFF, 0x15, 0x00000002, // FF15 00000002: CALL [RIP+8]
		0xEB, 0x08,             // EB 08:         JMP +10
		0x0000000000000000ULL   // Absolute destination address
	};
	JMP_ABS jmp = {
		0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
		0x0000000000000000ULL   // Absolute destination address
	};
	JCC_ABS jcc = {
		0x70, 0x0E,             // 7* 0E:         J** +16
		0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
		0x0000000000000000ULL   // Absolute destination address
	};
#else
	CALL_REL call = {
		0xE8,                   // E8 xxxxxxxx: CALL +5+xxxxxxxx
		0x00000000              // Relative destination address
	};
	JMP_REL jmp = {
		0xE9,                   // E9 xxxxxxxx: JMP +5+xxxxxxxx
		0x00000000              // Relative destination address
	};
	JCC_REL jcc = {
		0x0F, 0x80,             // 0F8* xxxxxxxx: J** +6+xxxxxxxx
		0x00000000              // Relative destination address
	};
#endif

	UINT8     oldPos = 0;
	UINT8     newPos = 0;
	ULONG_PTR jmpDest = 0;     // Destination address of an internal jump.
	BOOL      finished = FALSE; // Is the function completed?
#ifdef _M_X64
	UINT8     instBuf[16];
#endif

	ct->patchAbove = FALSE;
	ct->nIP = 0;

	do
	{
		HDE       hs;
		UINT      copySize;
		LPVOID    pCopySrc;
		ULONG_PTR pOldInst = (ULONG_PTR)ct->pTarget + oldPos;
		ULONG_PTR pNewInst = (ULONG_PTR)ct->pTrampoline + newPos;

		copySize = HDE_DISASM((LPVOID)pOldInst, &hs);
		if (hs.flags & F_ERROR)
			return FALSE;

		pCopySrc = (LPVOID)pOldInst;
		if (oldPos >= sizeof(JMP_REL))
		{
			// The trampoline function is long enough.
			// Complete the function with the jump to the target function.
#ifdef _M_X64
			jmp.address = pOldInst;
#else
			jmp.operand = (UINT32)(pOldInst - (pNewInst + sizeof(jmp)));
#endif
			pCopySrc = &jmp;
			copySize = sizeof(jmp);

			finished = TRUE;
		}
#ifdef _M_X64
		else if ((hs.modrm & 0xC7) == 0x05)
		{
			// Instructions using RIP relative addressing. (ModR/M = 00???101B)

			// Modify the RIP relative address.
			PUINT32 pRelAddr;

			// Avoid using memcpy to reduce the footprint.
			__movsb(instBuf, (LPBYTE)pOldInst, copySize);
			pCopySrc = instBuf;

			// Relative address is stored at (instruction length - immediate value length - 4).
			pRelAddr = (PUINT32)(instBuf + hs.len - ((hs.flags & 0x3C) >> 2) - 4);
			*pRelAddr
				= (UINT32)((pOldInst + hs.len + (INT32)hs.disp.disp32) - (pNewInst + hs.len));

			// Complete the function if JMP (FF /4).
			if (hs.opcode == 0xFF && hs.modrm_reg == 4)
				finished = TRUE;
		}
#endif
		else if (hs.opcode == 0xE8)
		{
			// Direct relative CALL
			ULONG_PTR dest = pOldInst + hs.len + (INT32)hs.imm.imm32;
#ifdef _M_X64
			call.address = dest;
#else
			call.operand = (UINT32)(dest - (pNewInst + sizeof(call)));
#endif
			pCopySrc = &call;
			copySize = sizeof(call);
		}
		else if ((hs.opcode & 0xFD) == 0xE9)
		{
			// Direct relative JMP (EB or E9)
			ULONG_PTR dest = pOldInst + hs.len;

			if (hs.opcode == 0xEB) // isShort jmp
				dest += (INT8)hs.imm.imm8;
			else
				dest += (INT32)hs.imm.imm32;

			// Simply copy an internal jump.
			if ((ULONG_PTR)ct->pTarget <= dest
				&& dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
			{
				if (jmpDest < dest)
					jmpDest = dest;
			}
			else
			{
#ifdef _M_X64
				jmp.address = dest;
#else
				jmp.operand = (UINT32)(dest - (pNewInst + sizeof(jmp)));
#endif
				pCopySrc = &jmp;
				copySize = sizeof(jmp);

				// Exit the function If it is not in the branch
				finished = (pOldInst >= jmpDest);
			}
		}
		else if ((hs.opcode & 0xF0) == 0x70
			|| (hs.opcode & 0xFC) == 0xE0
			|| (hs.opcode2 & 0xF0) == 0x80)
		{
			// Direct relative Jcc
			ULONG_PTR dest = pOldInst + hs.len;

			if ((hs.opcode & 0xF0) == 0x70      // Jcc
				|| (hs.opcode & 0xFC) == 0xE0)  // LOOPNZ/LOOPZ/LOOP/JECXZ
				dest += (INT8)hs.imm.imm8;
			else
				dest += (INT32)hs.imm.imm32;

			// Simply copy an internal jump.
			if ((ULONG_PTR)ct->pTarget <= dest
				&& dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
			{
				if (jmpDest < dest)
					jmpDest = dest;
			}
			else if ((hs.opcode & 0xFC) == 0xE0)
			{
				// LOOPNZ/LOOPZ/LOOP/JCXZ/JECXZ to the outside are not supported.
				return FALSE;
			}
			else
			{
				UINT8 cond = ((hs.opcode != 0x0F ? hs.opcode : hs.opcode2) & 0x0F);
#ifdef _M_X64
				// Invert the condition.
				jcc.opcode = 0x71 ^ cond;
				jcc.address = dest;
#else
				jcc.opcode1 = 0x80 | cond;
				jcc.operand = (UINT32)(dest - (pNewInst + sizeof(jcc)));
#endif
				pCopySrc = &jcc;
				copySize = sizeof(jcc);
			}
		}
		else if ((hs.opcode & 0xFE) == 0xC2)
		{
			// RET (C2 or C3)

			// Complete the function if not in a branch.
			finished = (pOldInst >= jmpDest);
		}

		// Can't alter the instruction length in a branch.
		if (pOldInst < jmpDest && copySize != hs.len)
			return FALSE;

		if ((newPos + copySize) > TRAMPOLINE_MAX_SIZE)
			return FALSE;

		if (ct->nIP >= ARRAYSIZE(ct->oldIPs))
			return FALSE;

		ct->oldIPs[ct->nIP] = oldPos;
		ct->newIPs[ct->nIP] = newPos;
		ct->nIP++;

		// Avoid using memcpy to reduce the footprint.
		__movsb((LPBYTE)ct->pTrampoline + newPos, (const BYTE*)pCopySrc, copySize);
		newPos += copySize;
		oldPos += hs.len;
	} while (!finished);

	// Is there enough place for a long jump?
	if (oldPos < sizeof(JMP_REL)
		&& !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL) - oldPos))
	{
		// Is there enough place for a short jump?
		if (oldPos < sizeof(JMP_REL_SHORT)
			&& !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL_SHORT) - oldPos))
		{
			return FALSE;
		}

		// Can we place the long jump above the function?
		if (!IsExecutableAddress((LPBYTE)ct->pTarget - sizeof(JMP_REL)))
			return FALSE;

		if (!IsCodePadding((LPBYTE)ct->pTarget - sizeof(JMP_REL), sizeof(JMP_REL)))
			return FALSE;

		ct->patchAbove = TRUE;
	}

#ifdef _M_X64
	// Create a relay function.
	jmp.address = (ULONG_PTR)ct->pDetour;

	ct->pRelay = (LPBYTE)ct->pTrampoline + newPos;
	memcpy(ct->pRelay, &jmp, sizeof(jmp));
#endif

	return TRUE;
}
