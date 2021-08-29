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

#ifdef _M_X64
unsigned char hde64_table[] = {
  0xa5,0xaa,0xa5,0xb8,0xa5,0xaa,0xa5,0xaa,0xa5,0xb8,0xa5,0xb8,0xa5,0xb8,0xa5,
  0xb8,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xac,0xc0,0xcc,0xc0,0xa1,0xa1,
  0xa1,0xa1,0xb1,0xa5,0xa5,0xa6,0xc0,0xc0,0xd7,0xda,0xe0,0xc0,0xe4,0xc0,0xea,
  0xea,0xe0,0xe0,0x98,0xc8,0xee,0xf1,0xa5,0xd3,0xa5,0xa5,0xa1,0xea,0x9e,0xc0,
  0xc0,0xc2,0xc0,0xe6,0x03,0x7f,0x11,0x7f,0x01,0x7f,0x01,0x3f,0x01,0x01,0xab,
  0x8b,0x90,0x64,0x5b,0x5b,0x5b,0x5b,0x5b,0x92,0x5b,0x5b,0x76,0x90,0x92,0x92,
  0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x6a,0x73,0x90,
  0x5b,0x52,0x52,0x52,0x52,0x5b,0x5b,0x5b,0x5b,0x77,0x7c,0x77,0x85,0x5b,0x5b,
  0x70,0x5b,0x7a,0xaf,0x76,0x76,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,
  0x5b,0x5b,0x86,0x01,0x03,0x01,0x04,0x03,0xd5,0x03,0xd5,0x03,0xcc,0x01,0xbc,
  0x03,0xf0,0x03,0x03,0x04,0x00,0x50,0x50,0x50,0x50,0xff,0x20,0x20,0x20,0x20,
  0x01,0x01,0x01,0x01,0xc4,0x02,0x10,0xff,0xff,0xff,0x01,0x00,0x03,0x11,0xff,
  0x03,0xc4,0xc6,0xc8,0x02,0x10,0x00,0xff,0xcc,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x01,0x01,0x03,0x01,0xff,0xff,0xc0,0xc2,0x10,0x11,0x02,0x03,0x01,0x01,
  0x01,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0x10,
  0x10,0x10,0x10,0x02,0x10,0x00,0x00,0xc6,0xc8,0x02,0x02,0x02,0x02,0x06,0x00,
  0x04,0x00,0x02,0xff,0x00,0xc0,0xc2,0x01,0x01,0x03,0x03,0x03,0xca,0x40,0x00,
  0x0a,0x00,0x04,0x00,0x00,0x00,0x00,0x7f,0x00,0x33,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0xff,0xbf,0xff,0xff,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0xff,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
  0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,
  0xff,0x40,0x40,0x40,0x40,0x41,0x49,0x40,0x40,0x40,0x40,0x4c,0x42,0x40,0x40,
  0x40,0x40,0x40,0x40,0x40,0x40,0x4f,0x44,0x53,0x40,0x40,0x40,0x44,0x57,0x43,
  0x5c,0x40,0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
  0x40,0x40,0x64,0x66,0x6e,0x6b,0x40,0x40,0x6a,0x46,0x40,0x40,0x44,0x46,0x40,
  0x40,0x5b,0x44,0x40,0x40,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x01,0x06,
  0x06,0x02,0x06,0x06,0x00,0x06,0x00,0x0a,0x0a,0x00,0x00,0x00,0x02,0x07,0x07,
  0x06,0x02,0x0d,0x06,0x06,0x06,0x0e,0x05,0x05,0x02,0x02,0x00,0x00,0x04,0x04,
  0x04,0x04,0x05,0x06,0x06,0x06,0x00,0x00,0x00,0x0e,0x00,0x00,0x08,0x00,0x10,
  0x00,0x18,0x00,0x20,0x00,0x28,0x00,0x30,0x00,0x80,0x01,0x82,0x01,0x86,0x00,
  0xf6,0xcf,0xfe,0x3f,0xab,0x00,0xb0,0x00,0xb1,0x00,0xb3,0x00,0xba,0xf8,0xbb,
  0x00,0xc0,0x00,0xc1,0x00,0xc7,0xbf,0x62,0xff,0x00,0x8d,0xff,0x00,0xc4,0xff,
  0x00,0xc5,0xff,0x00,0xff,0xff,0xeb,0x01,0xff,0x0e,0x12,0x08,0x00,0x13,0x09,
  0x00,0x16,0x08,0x00,0x17,0x09,0x00,0x2b,0x09,0x00,0xae,0xff,0x07,0xb2,0xff,
  0x00,0xb4,0xff,0x00,0xb5,0xff,0x00,0xc3,0x01,0x00,0xc7,0xff,0xbf,0xe7,0x08,
  0x00,0xf0,0x02,0x00
};

unsigned int hde64_disasm(const void* code, hde64s* hs)
{
	uint8_t x, c, * p = (uint8_t*)code, cflags, opcode, pref = 0;
	uint8_t* ht = hde64_table, m_mod, m_reg, m_rm, disp_size = 0;
	uint8_t op64 = 0;

	memset(hs, 0, sizeof(hde64s));

	for (x = 16; x; x--)
		switch (c = *p++) {
		case 0xf3:
			hs->p_rep = c;
			pref |= PRE_F3;
			break;
		case 0xf2:
			hs->p_rep = c;
			pref |= PRE_F2;
			break;
		case 0xf0:
			hs->p_lock = c;
			pref |= PRE_LOCK;
			break;
		case 0x26: case 0x2e: case 0x36:
		case 0x3e: case 0x64: case 0x65:
			hs->p_seg = c;
			pref |= PRE_SEG;
			break;
		case 0x66:
			hs->p_66 = c;
			pref |= PRE_66;
			break;
		case 0x67:
			hs->p_67 = c;
			pref |= PRE_67;
			break;
		default:
			goto pref_done;
		}
pref_done:

	hs->flags = (uint32_t)pref << 23;

	if (!pref)
		pref |= PRE_NONE;

	if ((c & 0xf0) == 0x40) {
		hs->flags |= F_PREFIX_REX;
		if ((hs->rex_w = (c & 0xf) >> 3) && (*p & 0xf8) == 0xb8)
			op64++;
		hs->rex_r = (c & 7) >> 2;
		hs->rex_x = (c & 3) >> 1;
		hs->rex_b = c & 1;
		if (((c = *p++) & 0xf0) == 0x40) {
			opcode = c;
			goto error_opcode;
		}
	}

	if ((hs->opcode = c) == 0x0f) {
		hs->opcode2 = c = *p++;
		ht += DELTA_OPCODES;
	}
	else if (c >= 0xa0 && c <= 0xa3) {
		op64++;
		if (pref & PRE_67)
			pref |= PRE_66;
		else
			pref &= ~PRE_66;
	}

	opcode = c;
	cflags = ht[ht[opcode / 4] + (opcode % 4)];

	if (cflags == C_ERROR) {
	error_opcode:
		hs->flags |= F_ERROR | F_ERROR_OPCODE;
		cflags = 0;
		if ((opcode & -3) == 0x24)
			cflags++;
	}

	x = 0;
	if (cflags & C_GROUP) {
		uint16_t t;
		t = *(uint16_t*)(ht + (cflags & 0x7f));
		cflags = (uint8_t)t;
		x = (uint8_t)(t >> 8);
	}

	if (hs->opcode2) {
		ht = hde64_table + DELTA_PREFIXES;
		if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
			hs->flags |= F_ERROR | F_ERROR_OPCODE;
	}

	if (cflags & C_MODRM) {
		hs->flags |= F_MODRM;
		hs->modrm = c = *p++;
		hs->modrm_mod = m_mod = c >> 6;
		hs->modrm_rm = m_rm = c & 7;
		hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

		if (x && ((x << m_reg) & 0x80))
			hs->flags |= F_ERROR | F_ERROR_OPCODE;

		if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
			uint8_t t = opcode - 0xd9;
			if (m_mod == 3) {
				ht = hde64_table + DELTA_FPU_MODRM + t * 8;
				t = ht[m_reg] << m_rm;
			}
			else {
				ht = hde64_table + DELTA_FPU_REG;
				t = ht[t] << m_reg;
			}
			if (t & 0x80)
				hs->flags |= F_ERROR | F_ERROR_OPCODE;
		}

		if (pref & PRE_LOCK) {
			if (m_mod == 3) {
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			}
			else {
				uint8_t* table_end, op = opcode;
				if (hs->opcode2) {
					ht = hde64_table + DELTA_OP2_LOCK_OK;
					table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
				}
				else {
					ht = hde64_table + DELTA_OP_LOCK_OK;
					table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
					op &= -2;
				}
				for (; ht != table_end; ht++)
					if (*ht++ == op) {
						if (!((*ht << m_reg) & 0x80))
							goto no_lock_error;
						else
							break;
					}
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			no_lock_error:
				;
			}
		}

		if (hs->opcode2) {
			switch (opcode) {
			case 0x20: case 0x22:
				m_mod = 3;
				if (m_reg > 4 || m_reg == 1)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x21: case 0x23:
				m_mod = 3;
				if (m_reg == 4 || m_reg == 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}
		else {
			switch (opcode) {
			case 0x8c:
				if (m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x8e:
				if (m_reg == 1 || m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}

		if (m_mod == 3) {
			uint8_t* table_end;
			if (hs->opcode2) {
				ht = hde64_table + DELTA_OP2_ONLY_MEM;
				table_end = ht + sizeof(hde64_table) - DELTA_OP2_ONLY_MEM;
			}
			else {
				ht = hde64_table + DELTA_OP_ONLY_MEM;
				table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
			}
			for (; ht != table_end; ht += 2)
				if (*ht++ == opcode) {
					if (*ht++ & pref && !((*ht << m_reg) & 0x80))
						goto error_operand;
					else
						break;
				}
			goto no_error_operand;
		}
		else if (hs->opcode2) {
			switch (opcode) {
			case 0x50: case 0xd7: case 0xf7:
				if (pref & (PRE_NONE | PRE_66))
					goto error_operand;
				break;
			case 0xd6:
				if (pref & (PRE_F2 | PRE_F3))
					goto error_operand;
				break;
			case 0xc5:
				goto error_operand;
			}
			goto no_error_operand;
		}
		else
			goto no_error_operand;

	error_operand:
		hs->flags |= F_ERROR | F_ERROR_OPERAND;
	no_error_operand:

		c = *p++;
		if (m_reg <= 1) {
			if (opcode == 0xf6)
				cflags |= C_IMM8;
			else if (opcode == 0xf7)
				cflags |= C_IMM_P66;
		}

		switch (m_mod) {
		case 0:
			if (pref & PRE_67) {
				if (m_rm == 6)
					disp_size = 2;
			}
			else
				if (m_rm == 5)
					disp_size = 4;
			break;
		case 1:
			disp_size = 1;
			break;
		case 2:
			disp_size = 2;
			if (!(pref & PRE_67))
				disp_size <<= 1;
		}

		if (m_mod != 3 && m_rm == 4) {
			hs->flags |= F_SIB;
			p++;
			hs->sib = c;
			hs->sib_scale = c >> 6;
			hs->sib_index = (c & 0x3f) >> 3;
			if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
				disp_size = 4;
		}

		p--;
		switch (disp_size) {
		case 1:
			hs->flags |= F_DISP8;
			hs->disp.disp8 = *p;
			break;
		case 2:
			hs->flags |= F_DISP16;
			hs->disp.disp16 = *(uint16_t*)p;
			break;
		case 4:
			hs->flags |= F_DISP32;
			hs->disp.disp32 = *(uint32_t*)p;
		}
		p += disp_size;
	}
	else if (pref & PRE_LOCK)
		hs->flags |= F_ERROR | F_ERROR_LOCK;

	if (cflags & C_IMM_P66) {
		if (cflags & C_REL32) {
			if (pref & PRE_66) {
				hs->flags |= F_IMM16 | F_RELATIVE;
				hs->imm.imm16 = *(uint16_t*)p;
				p += 2;
				goto disasm_done;
			}
			goto rel32_ok;
		}
		if (op64) {
			hs->flags |= F_IMM64;
			hs->imm.imm64 = *(uint64_t*)p;
			p += 8;
		}
		else if (!(pref & PRE_66)) {
			hs->flags |= F_IMM32;
			hs->imm.imm32 = *(uint32_t*)p;
			p += 4;
		}
		else
			goto imm16_ok;
	}


	if (cflags & C_IMM16) {
	imm16_ok:
		hs->flags |= F_IMM16;
		hs->imm.imm16 = *(uint16_t*)p;
		p += 2;
	}
	if (cflags & C_IMM8) {
		hs->flags |= F_IMM8;
		hs->imm.imm8 = *p++;
	}

	if (cflags & C_REL32) {
	rel32_ok:
		hs->flags |= F_IMM32 | F_RELATIVE;
		hs->imm.imm32 = *(uint32_t*)p;
		p += 4;
	}
	else if (cflags & C_REL8) {
		hs->flags |= F_IMM8 | F_RELATIVE;
		hs->imm.imm8 = *p++;
	}

disasm_done:

	if ((hs->len = (uint8_t)(p - (uint8_t*)code)) > 15) {
		hs->flags |= F_ERROR | F_ERROR_LENGTH;
		hs->len = 15;
	}

	return (unsigned int)hs->len;
}
#else
unsigned char hde32_table[] = {
  0xa3,0xa8,0xa3,0xa8,0xa3,0xa8,0xa3,0xa8,0xa3,0xa8,0xa3,0xa8,0xa3,0xa8,0xa3,
  0xa8,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xac,0xaa,0xb2,0xaa,0x9f,0x9f,
  0x9f,0x9f,0xb5,0xa3,0xa3,0xa4,0xaa,0xaa,0xba,0xaa,0x96,0xaa,0xa8,0xaa,0xc3,
  0xc3,0x96,0x96,0xb7,0xae,0xd6,0xbd,0xa3,0xc5,0xa3,0xa3,0x9f,0xc3,0x9c,0xaa,
  0xaa,0xac,0xaa,0xbf,0x03,0x7f,0x11,0x7f,0x01,0x7f,0x01,0x3f,0x01,0x01,0x90,
  0x82,0x7d,0x97,0x59,0x59,0x59,0x59,0x59,0x7f,0x59,0x59,0x60,0x7d,0x7f,0x7f,
  0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x9a,0x88,0x7d,
  0x59,0x50,0x50,0x50,0x50,0x59,0x59,0x59,0x59,0x61,0x94,0x61,0x9e,0x59,0x59,
  0x85,0x59,0x92,0xa3,0x60,0x60,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,0x59,
  0x59,0x59,0x9f,0x01,0x03,0x01,0x04,0x03,0xd5,0x03,0xcc,0x01,0xbc,0x03,0xf0,
  0x10,0x10,0x10,0x10,0x50,0x50,0x50,0x50,0x14,0x20,0x20,0x20,0x20,0x01,0x01,
  0x01,0x01,0xc4,0x02,0x10,0x00,0x00,0x00,0x00,0x01,0x01,0xc0,0xc2,0x10,0x11,
  0x02,0x03,0x11,0x03,0x03,0x04,0x00,0x00,0x14,0x00,0x02,0x00,0x00,0xc6,0xc8,
  0x02,0x02,0x02,0x02,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0xca,
  0x01,0x01,0x01,0x00,0x06,0x00,0x04,0x00,0xc0,0xc2,0x01,0x01,0x03,0x01,0xff,
  0xff,0x01,0x00,0x03,0xc4,0xc4,0xc6,0x03,0x01,0x01,0x01,0xff,0x03,0x03,0x03,
  0xc8,0x40,0x00,0x0a,0x00,0x04,0x00,0x00,0x00,0x00,0x7f,0x00,0x33,0x01,0x00,
  0x00,0x00,0x00,0x00,0x00,0xff,0xbf,0xff,0xff,0x00,0x00,0x00,0x00,0x07,0x00,
  0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xff,0xff,0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x7f,0x00,0x00,0xff,0x4a,0x4a,0x4a,0x4a,0x4b,0x52,0x4a,0x4a,0x4a,0x4a,0x4f,
  0x4c,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x55,0x45,0x40,0x4a,0x4a,0x4a,
  0x45,0x59,0x4d,0x46,0x4a,0x5d,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,0x4a,
  0x4a,0x4a,0x4a,0x4a,0x4a,0x61,0x63,0x67,0x4e,0x4a,0x4a,0x6b,0x6d,0x4a,0x4a,
  0x45,0x6d,0x4a,0x4a,0x44,0x45,0x4a,0x4a,0x00,0x00,0x00,0x02,0x0d,0x06,0x06,
  0x06,0x06,0x0e,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x00,0x06,0x06,0x02,0x06,
  0x00,0x0a,0x0a,0x07,0x07,0x06,0x02,0x05,0x05,0x02,0x02,0x00,0x00,0x04,0x04,
  0x04,0x04,0x00,0x00,0x00,0x0e,0x05,0x06,0x06,0x06,0x01,0x06,0x00,0x00,0x08,
  0x00,0x10,0x00,0x18,0x00,0x20,0x00,0x28,0x00,0x30,0x00,0x80,0x01,0x82,0x01,
  0x86,0x00,0xf6,0xcf,0xfe,0x3f,0xab,0x00,0xb0,0x00,0xb1,0x00,0xb3,0x00,0xba,
  0xf8,0xbb,0x00,0xc0,0x00,0xc1,0x00,0xc7,0xbf,0x62,0xff,0x00,0x8d,0xff,0x00,
  0xc4,0xff,0x00,0xc5,0xff,0x00,0xff,0xff,0xeb,0x01,0xff,0x0e,0x12,0x08,0x00,
  0x13,0x09,0x00,0x16,0x08,0x00,0x17,0x09,0x00,0x2b,0x09,0x00,0xae,0xff,0x07,
  0xb2,0xff,0x00,0xb4,0xff,0x00,0xb5,0xff,0x00,0xc3,0x01,0x00,0xc7,0xff,0xbf,
  0xe7,0x08,0x00,0xf0,0x02,0x00
};

unsigned int hde32_disasm(const void* code, hde32s* hs)
{
	uint8_t x, c, * p = (uint8_t*)code, cflags, opcode, pref = 0;
	uint8_t* ht = hde32_table, m_mod, m_reg, m_rm, disp_size = 0;

	memset(hs, 0, sizeof(hde32s));

	for (x = 16; x; x--)
		switch (c = *p++) {
		case 0xf3:
			hs->p_rep = c;
			pref |= PRE_F3;
			break;
		case 0xf2:
			hs->p_rep = c;
			pref |= PRE_F2;
			break;
		case 0xf0:
			hs->p_lock = c;
			pref |= PRE_LOCK;
			break;
		case 0x26: case 0x2e: case 0x36:
		case 0x3e: case 0x64: case 0x65:
			hs->p_seg = c;
			pref |= PRE_SEG;
			break;
		case 0x66:
			hs->p_66 = c;
			pref |= PRE_66;
			break;
		case 0x67:
			hs->p_67 = c;
			pref |= PRE_67;
			break;
		default:
			goto pref_done;
		}
pref_done:

	hs->flags = (uint32_t)pref << 23;

	if (!pref)
		pref |= PRE_NONE;

	if ((hs->opcode = c) == 0x0f) {
		hs->opcode2 = c = *p++;
		ht += DELTA_OPCODES;
	}
	else if (c >= 0xa0 && c <= 0xa3) {
		if (pref & PRE_67)
			pref |= PRE_66;
		else
			pref &= ~PRE_66;
	}

	opcode = c;
	cflags = ht[ht[opcode / 4] + (opcode % 4)];

	if (cflags == C_ERROR) {
		hs->flags |= F_ERROR | F_ERROR_OPCODE;
		cflags = 0;
		if ((opcode & -3) == 0x24)
			cflags++;
	}

	x = 0;
	if (cflags & C_GROUP) {
		uint16_t t;
		t = *(uint16_t*)(ht + (cflags & 0x7f));
		cflags = (uint8_t)t;
		x = (uint8_t)(t >> 8);
	}

	if (hs->opcode2) {
		ht = hde32_table + DELTA_PREFIXES;
		if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
			hs->flags |= F_ERROR | F_ERROR_OPCODE;
	}

	if (cflags & C_MODRM) {
		hs->flags |= F_MODRM;
		hs->modrm = c = *p++;
		hs->modrm_mod = m_mod = c >> 6;
		hs->modrm_rm = m_rm = c & 7;
		hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

		if (x && ((x << m_reg) & 0x80))
			hs->flags |= F_ERROR | F_ERROR_OPCODE;

		if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
			uint8_t t = opcode - 0xd9;
			if (m_mod == 3) {
				ht = hde32_table + DELTA_FPU_MODRM + t * 8;
				t = ht[m_reg] << m_rm;
			}
			else {
				ht = hde32_table + DELTA_FPU_REG;
				t = ht[t] << m_reg;
			}
			if (t & 0x80)
				hs->flags |= F_ERROR | F_ERROR_OPCODE;
		}

		if (pref & PRE_LOCK) {
			if (m_mod == 3) {
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			}
			else {
				uint8_t* table_end, op = opcode;
				if (hs->opcode2) {
					ht = hde32_table + DELTA_OP2_LOCK_OK;
					table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
				}
				else {
					ht = hde32_table + DELTA_OP_LOCK_OK;
					table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
					op &= -2;
				}
				for (; ht != table_end; ht++)
					if (*ht++ == op) {
						if (!((*ht << m_reg) & 0x80))
							goto no_lock_error;
						else
							break;
					}
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			no_lock_error:
				;
			}
		}

		if (hs->opcode2) {
			switch (opcode) {
			case 0x20: case 0x22:
				m_mod = 3;
				if (m_reg > 4 || m_reg == 1)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x21: case 0x23:
				m_mod = 3;
				if (m_reg == 4 || m_reg == 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}
		else {
			switch (opcode) {
			case 0x8c:
				if (m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x8e:
				if (m_reg == 1 || m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}

		if (m_mod == 3) {
			uint8_t* table_end;
			if (hs->opcode2) {
				ht = hde32_table + DELTA_OP2_ONLY_MEM;
				table_end = ht + sizeof(hde32_table) - DELTA_OP2_ONLY_MEM;
			}
			else {
				ht = hde32_table + DELTA_OP_ONLY_MEM;
				table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
			}
			for (; ht != table_end; ht += 2)
				if (*ht++ == opcode) {
					if ((*ht++ & pref) && !((*ht << m_reg) & 0x80))
						goto error_operand;
					else
						break;
				}
			goto no_error_operand;
		}
		else if (hs->opcode2) {
			switch (opcode) {
			case 0x50: case 0xd7: case 0xf7:
				if (pref & (PRE_NONE | PRE_66))
					goto error_operand;
				break;
			case 0xd6:
				if (pref & (PRE_F2 | PRE_F3))
					goto error_operand;
				break;
			case 0xc5:
				goto error_operand;
			}
			goto no_error_operand;
		}
		else
			goto no_error_operand;

	error_operand:
		hs->flags |= F_ERROR | F_ERROR_OPERAND;
	no_error_operand:

		c = *p++;
		if (m_reg <= 1) {
			if (opcode == 0xf6)
				cflags |= C_IMM8;
			else if (opcode == 0xf7)
				cflags |= C_IMM_P66;
		}

		switch (m_mod) {
		case 0:
			if (pref & PRE_67) {
				if (m_rm == 6)
					disp_size = 2;
			}
			else
				if (m_rm == 5)
					disp_size = 4;
			break;
		case 1:
			disp_size = 1;
			break;
		case 2:
			disp_size = 2;
			if (!(pref & PRE_67))
				disp_size <<= 1;
			break;
		}

		if (m_mod != 3 && m_rm == 4 && !(pref & PRE_67)) {
			hs->flags |= F_SIB;
			p++;
			hs->sib = c;
			hs->sib_scale = c >> 6;
			hs->sib_index = (c & 0x3f) >> 3;
			if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
				disp_size = 4;
		}

		p--;
		switch (disp_size) {
		case 1:
			hs->flags |= F_DISP8;
			hs->disp.disp8 = *p;
			break;
		case 2:
			hs->flags |= F_DISP16;
			hs->disp.disp16 = *(uint16_t*)p;
			break;
		case 4:
			hs->flags |= F_DISP32;
			hs->disp.disp32 = *(uint32_t*)p;
			break;
		}
		p += disp_size;
	}
	else if (pref & PRE_LOCK)
		hs->flags |= F_ERROR | F_ERROR_LOCK;

	if (cflags & C_IMM_P66) {
		if (cflags & C_REL32) {
			if (pref & PRE_66) {
				hs->flags |= F_IMM16 | F_RELATIVE;
				hs->imm.imm16 = *(uint16_t*)p;
				p += 2;
				goto disasm_done;
			}
			goto rel32_ok;
		}
		if (pref & PRE_66) {
			hs->flags |= F_IMM16;
			hs->imm.imm16 = *(uint16_t*)p;
			p += 2;
		}
		else {
			hs->flags |= F_IMM32;
			hs->imm.imm32 = *(uint32_t*)p;
			p += 4;
		}
	}

	if (cflags & C_IMM16) {
		if (hs->flags & F_IMM32) {
			hs->flags |= F_IMM16;
			hs->disp.disp16 = *(uint16_t*)p;
		}
		else if (hs->flags & F_IMM16) {
			hs->flags |= F_2IMM16;
			hs->disp.disp16 = *(uint16_t*)p;
		}
		else {
			hs->flags |= F_IMM16;
			hs->imm.imm16 = *(uint16_t*)p;
		}
		p += 2;
	}
	if (cflags & C_IMM8) {
		hs->flags |= F_IMM8;
		hs->imm.imm8 = *p++;
	}

	if (cflags & C_REL32) {
	rel32_ok:
		hs->flags |= F_IMM32 | F_RELATIVE;
		hs->imm.imm32 = *(uint32_t*)p;
		p += 4;
	}
	else if (cflags & C_REL8) {
		hs->flags |= F_IMM8 | F_RELATIVE;
		hs->imm.imm8 = *p++;
	}

disasm_done:

	if ((hs->len = (uint8_t)(p - (uint8_t*)code)) > 15) {
		hs->flags |= F_ERROR | F_ERROR_LENGTH;
		hs->len = 15;
	}

	return (unsigned int)hs->len;
}
#endif // _M_X64


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

#ifdef _M_X64
static LPVOID FindPrevFreeRegion(LPVOID pAddress, LPVOID pMinAddr, DWORD dwAllocationGranularity)
{
	ULONG_PTR tryAddr = (ULONG_PTR)pAddress;

	// Round down to the next allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the previous allocation granularity multiply.
	tryAddr -= dwAllocationGranularity;

	while (tryAddr >= (ULONG_PTR)pMinAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		if ((ULONG_PTR)mbi.AllocationBase < dwAllocationGranularity)
			break;

		tryAddr = (ULONG_PTR)mbi.AllocationBase - dwAllocationGranularity;
	}

	return NULL;
}

static LPVOID FindNextFreeRegion(LPVOID pAddress, LPVOID pMaxAddr, DWORD dwAllocationGranularity)
{
	ULONG_PTR tryAddr = (ULONG_PTR)pAddress;

	// Round down to the next allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the next allocation granularity multiply.
	tryAddr += dwAllocationGranularity;

	while (tryAddr <= (ULONG_PTR)pMaxAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		tryAddr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;

		// Round up to the next allocation granularity.
		tryAddr += dwAllocationGranularity - 1;
		tryAddr -= tryAddr % dwAllocationGranularity;
	}

	return NULL;
}
#endif

//-------------------------------------------------------------------------
static PMEMORY_BLOCK GetMemoryBlock(LPVOID pOrigin)
{
	PMEMORY_BLOCK pBlock;
#ifdef _M_X64
	ULONG_PTR minAddr;
	ULONG_PTR maxAddr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (ULONG_PTR)si.lpMinimumApplicationAddress;
	maxAddr = (ULONG_PTR)si.lpMaximumApplicationAddress;

	// pOrigin ?512MB
	if ((ULONG_PTR)pOrigin > MAX_MEMORY_RANGE && minAddr < (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE)
		minAddr = (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE;

	if (maxAddr >(ULONG_PTR)pOrigin + MAX_MEMORY_RANGE)
		maxAddr = (ULONG_PTR)pOrigin + MAX_MEMORY_RANGE;

	// Make room for MEMORY_BLOCK_SIZE bytes.
	maxAddr -= MEMORY_BLOCK_SIZE - 1;
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

#ifdef _M_X64
	// Alloc a new block above if not found.
	{
		LPVOID pAlloc = pOrigin;
		while ((ULONG_PTR)pAlloc >= minAddr)
		{
			pAlloc = FindPrevFreeRegion(pAlloc, (LPVOID)minAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}

	// Alloc a new block below if not found.
	if (pBlock == NULL)
	{
		LPVOID pAlloc = pOrigin;
		while ((ULONG_PTR)pAlloc <= maxAddr)
		{
			pAlloc = FindNextFreeRegion(pAlloc, (LPVOID)maxAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}
#else
	// In x86 mode, a memory block can be placed anywhere.
	pBlock = (PMEMORY_BLOCK)VirtualAlloc(
		NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif

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

PHOOK_ENTRY MinHookEnable(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal)
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
	pHook = (PHOOK_ENTRY)malloc(sizeof(HOOK_ENTRY));
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

	if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		FreeBuffer(pBuffer);
		free(pHook);
		return NULL;
	}

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

	// Just-in-case measure.
	FlushInstructionCache(GetCurrentProcess(), pPatchTarget, patchSize);

	if (ppOriginal)
		*ppOriginal = pHook->pTrampoline;
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

	if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		free(pHook);
		return FALSE;
	}
	if (pHook->patchAbove)
		memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
	else
		memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL));
	VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);
	free(pHook);
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
				// Invert the condition in x64 mode to simplify the conditional jump logic.
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

		// Trampoline function is too large.
		if ((newPos + copySize) > TRAMPOLINE_MAX_SIZE)
			return FALSE;

		// Trampoline function has too many instructions.
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
