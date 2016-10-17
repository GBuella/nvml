/*
 * Copyright 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "intercept.h"
#include "intercept_util.h"
#include "../../libpmem/cpu.h"

#include <cpuid.h>
#include <stdint.h>
#include <syscall.h>
#include <sys/mman.h>
#include <string.h>

#include <stdio.h>

#define PAGE_SIZE ((size_t)0x1000)

static unsigned char *
round_down_address(unsigned char *address)
{
	return (unsigned char *)(((uintptr_t)address) & ~(PAGE_SIZE - 1));
}


static unsigned char asm_wrapper_space[0x100000];

static unsigned char *next_asm_wrapper_space(void);

static void create_wrapper(struct patch_desc *patch, void *dest_routine);

/*
 * create_jump(opcode, from, to)
 * Create a 5 byte jmp/call instruction jumping to address to, by overwriting
 * code starting at address from.
 */
void
create_jump(unsigned char opcode, unsigned char *from, void *to)
{
	/*
	 * The operand is the difference between the
	 * instruction pointer pointing to the instruction
	 * just after the call, and the to address.
	 * Thus RIP seen by the call instruction is from + 5
	 */
	ptrdiff_t delta = ((unsigned char *)to) - (from + JUMP_INS_SIZE);

	if (delta > ((ptrdiff_t)INT32_MAX) || delta < ((ptrdiff_t)INT32_MIN))
		xabort(); // belt and suspenders

	int32_t delta32 = (int32_t)delta;
	unsigned char *d = (unsigned char *)&delta32;

	from[0] = opcode;
	from[1] = d[0];
	from[2] = d[1];
	from[3] = d[2];
	from[4] = d[3];
}


void
create_patch_wrappers(struct intercept_desc *desc)
{
	for (unsigned patch_i = 0; patch_i < desc->count; ++patch_i) {
		struct patch_desc *patch = desc->items + patch_i;

		if (patch->padding_addr != NULL) {
			patch->uses_padding = true;
			patch->uses_prev_ins = false;
			patch->uses_next_ins = false;
			patch->dst_jmp_patch = patch->padding_addr;
			patch->return_address =
			    patch->syscall_addr + SYSCALL_INS_SIZE;
			patch->ok = true;
		} else {
			patch->uses_padding = false;

			patch->uses_prev_ins =
			    !(patch->preceding_ins.has_ip_relative_opr ||
			    patch->preceding_ins.is_rel_jump ||
			    patch->preceding_ins.is_ret ||
			    has_jump(desc, patch->syscall_addr));

			patch->uses_prev_ins_2 = patch->uses_prev_ins &&
			    !(patch->preceding_ins_2.has_ip_relative_opr ||
			    patch->preceding_ins_2.is_rel_jump ||
			    patch->preceding_ins_2.is_ret ||
			    has_jump(desc, patch->syscall_addr
				- patch->preceding_ins.length));

			patch->uses_next_ins =
			    !(patch->following_ins.has_ip_relative_opr ||
			    patch->following_ins.is_rel_jump ||
			    has_jump(desc,
				patch->syscall_addr + SYSCALL_INS_SIZE));

			unsigned length = SYSCALL_INS_SIZE;

			patch->dst_jmp_patch = patch->syscall_addr;

			if (patch->uses_prev_ins) {
				length += patch->preceding_ins.length;
				patch->dst_jmp_patch -=
				    patch->preceding_ins.length;

				if (patch->uses_prev_ins_2) {
					length += patch->preceding_ins_2.length;
					patch->dst_jmp_patch -=
					    patch->preceding_ins_2.length;
				}
			}

			if (patch->uses_next_ins) {
				length += patch->following_ins.length;
				patch->return_address = patch->syscall_addr +
				    SYSCALL_INS_SIZE +
				    patch->following_ins.length;
			} else {
				patch->return_address =
					patch->syscall_addr + SYSCALL_INS_SIZE;
			}

			patch->ok = length >= JUMP_INS_SIZE;

			if (length < JUMP_INS_SIZE) {
				char buffer[0x1000];

				int l = sprintf(buffer,
					"unintercepted syscall at: %s 0x%lx\n",
					desc->dlinfo.dli_fname,
					patch->syscall_offset);

				intercept_log(buffer, (size_t)l);
				continue;
			}
		}

		create_wrapper(patch, desc->c_detination);
	}

}

extern unsigned char intercept_asm_wrapper_tmpl[];
extern unsigned char intercept_asm_wrapper_end;
extern unsigned char intercept_asm_wrapper_prefix;
extern unsigned char intercept_asm_wrapper_postfix;
extern unsigned char intercept_asm_wrapper_call;
extern unsigned char intercept_asm_wrapper_simd_save;
extern unsigned char intercept_asm_wrapper_simd_restore;
extern unsigned char intercept_asm_wrapper_return_jump;
extern unsigned char intercept_asm_wrapper_push_origin_addr;
extern unsigned char intercept_asm_wrapper_simd_save_YMM;
extern unsigned char intercept_asm_wrapper_simd_save_YMM_end;
extern unsigned char intercept_asm_wrapper_simd_restore_YMM;
extern unsigned char intercept_asm_wrapper_simd_restore_YMM_end;

static size_t tmpl_size;
static ptrdiff_t o_prefix;
static ptrdiff_t o_postfix;
static ptrdiff_t o_call;
static ptrdiff_t o_ret_jump;
static ptrdiff_t o_push_origin;
static ptrdiff_t o_simd_save;
static ptrdiff_t o_simd_restore;
static size_t simd_save_YMM_size;
static size_t simd_restore_YMM_size;

static bool must_save_ymm_registers;

void
init_patcher(void)
{
	unsigned char *begin = &intercept_asm_wrapper_tmpl[0];

	if (&intercept_asm_wrapper_end <= begin)
		xabort();

	tmpl_size = (size_t)(&intercept_asm_wrapper_end - begin);
	o_prefix = &intercept_asm_wrapper_prefix - begin;
	o_postfix = &intercept_asm_wrapper_postfix - begin;
	o_call = &intercept_asm_wrapper_call - begin;
	o_ret_jump = &intercept_asm_wrapper_return_jump - begin;
	o_push_origin = &intercept_asm_wrapper_push_origin_addr - begin;
	o_simd_save = &intercept_asm_wrapper_simd_save - begin;
	o_simd_restore = &intercept_asm_wrapper_simd_restore - begin;
	simd_save_YMM_size = (size_t)(&intercept_asm_wrapper_simd_save_YMM_end -
	    &intercept_asm_wrapper_simd_save_YMM);
	simd_restore_YMM_size =
	    (size_t)(&intercept_asm_wrapper_simd_restore_YMM_end -
	    &intercept_asm_wrapper_simd_restore_YMM);

	must_save_ymm_registers = has_ymm_registers();
}

static void
copy_ymm_handler_code(unsigned char *asm_wrapper)
{
	memcpy(asm_wrapper + o_simd_save,
	    &intercept_asm_wrapper_simd_save_YMM, simd_save_YMM_size);
	memcpy(asm_wrapper + o_simd_restore,
	    &intercept_asm_wrapper_simd_restore_YMM, simd_restore_YMM_size);
}

static void
create_push_imm(unsigned char *push, uint32_t syscall_offset)
{
	push[0] = PUSH_IMM_OPCODE;
	push[1] = (unsigned char)(syscall_offset % 0x100);
	push[2] = (unsigned char)((syscall_offset / 0x100) % 0x100);
	push[3] = (unsigned char)((syscall_offset / 0x10000) % 0x100);
	push[4] = (unsigned char)((syscall_offset / 0x1000000) % 0x100);
}

static void
create_wrapper(struct patch_desc *patch, void *dest_routine)
{
	unsigned char *begin;

	/* Create a new copy of the template */
	patch->asm_wrapper = begin = next_asm_wrapper_space();
	memcpy(begin, intercept_asm_wrapper_tmpl, tmpl_size);

	/* Copy the prev/next instrucions, if they are copiable */
	if (patch->uses_prev_ins) {
		size_t length = patch->preceding_ins.length;
		if (patch->uses_prev_ins_2)
			length += patch->preceding_ins_2.length;

		memcpy(begin + o_prefix, patch->syscall_addr - length, length);
	}
	if (patch->uses_next_ins) {
		memcpy(begin + o_postfix,
		    patch->syscall_addr + SYSCALL_INS_SIZE,
		    patch->following_ins.length);
	}

	if (patch->syscall_offset > UINT32_MAX)
		xabort(); // libc larger than 2 gigabytes? wow

	/* the instruction pushing the syscall's address to the stack */
	create_push_imm(begin + o_push_origin, (uint32_t)patch->syscall_offset);

	/* Create the jump instrucions returning to the original code */
	create_jump(JMP_OPCODE, begin + o_ret_jump, patch->return_address);

	/* Create the call instrucions calling the intended C function */
	create_jump(CALL_OPCODE, begin + o_call, dest_routine);

	if (must_save_ymm_registers)
		copy_ymm_handler_code(begin);
}

static void
create_short_jump(unsigned char *from, unsigned char *to)
{
	ptrdiff_t d = to - (from + 2);

	// that is "d < -128"
	// gotta fix cstyle...
	if (d < - 128 || d > 127)
		xabort();

	from[0] = SHORT_JMP_OPCODE;
	from[1] = (unsigned char)((char)d);
}

/*
 * activate_patches()
 * Loop over all the patches, and and overwrite each syscall.
 */
void
activate_patches(struct intercept_desc *desc)
{
	unsigned char *first_page;
	size_t size;

	if (desc->count == 0)
		return;

	first_page = round_down_address(desc->text_start);
	size = (size_t)(desc->text_end - first_page);

	if (syscall_no_intercept(SYS_mprotect, first_page, size,
	    PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
		xabort();

	for (unsigned i = 0; i < desc->count; ++i) {
		const struct patch_desc *patch = desc->items + i;

		if (!patch->ok)
			continue;

		if (patch->dst_jmp_patch < desc->text_start ||
		    patch->dst_jmp_patch > desc->text_end)
			xabort(); // belt and suspenders

		create_jump(JMP_OPCODE,
		    patch->dst_jmp_patch, patch->asm_wrapper);

		if (patch->uses_padding) {
			create_short_jump(patch->syscall_addr,
			    patch->dst_jmp_patch);
		} else {
			unsigned char *byte;

			for (byte = patch->dst_jmp_patch + JUMP_INS_SIZE;
				byte < patch->return_address;
				++byte) {
				*byte = NOP_OPCODE;
			}
		}
	}

	if (syscall_no_intercept(SYS_mprotect, first_page, size,
	    PROT_READ | PROT_EXEC) != 0)
		xabort();
}

static unsigned char *
next_asm_wrapper_space(void)
{
	static size_t next = 0x1000;

	unsigned char *result;

	if (next + tmpl_size + PAGE_SIZE > sizeof(asm_wrapper_space))
		xabort();

	result = asm_wrapper_space + next;

	next += tmpl_size;

	return result;
}

void
mprotect_asm_wrappers(void)
{
	if (syscall_no_intercept(SYS_mprotect,
	    round_down_address(asm_wrapper_space + PAGE_SIZE),
	    sizeof(asm_wrapper_space) - PAGE_SIZE,
	    PROT_READ | PROT_EXEC) != 0)
		xabort();
}
