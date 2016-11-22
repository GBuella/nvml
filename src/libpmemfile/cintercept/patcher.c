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

static void create_wrapper(struct patch_desc *patch, void *dest_routine,
			bool use_absolute_return,
			const char *libpath);

/*
 * create_absolute_jump(from, to)
 * Create an indirect jump, with the pointer right next to the instruction.
 *
 * jmp *0(%rip)
 *
 * This uses up 6 bytes for the jump instruction, and another 8 bytes
 * for the pointer right after the instruction.
 */
static void
create_absolute_jump(unsigned char *from, void *to)
{
	from[0] = 0xff; // opcode of RIP based indirect jump
	from[1] = 0x25; // opcode of RIP based indirect jump
	from[2] = 0; // 32 bit zero offset
	from[3] = 0; // this means zero relative to the value
	from[4] = 0; // of RIP, which during the execution of the jump
	from[5] = 0; // points to right after the jump instruction

	unsigned char *d = (unsigned char *)&to;

	from[6] = d[0]; // so, this is where (RIP + 0) points to,
	from[7] = d[1]; // jump reads the destination address
	from[8] = d[2]; // from here
	from[9] = d[3];
	from[10] = d[4];
	from[11] = d[5];
	from[12] = d[6];
	from[13] = d[7];

	// Just written 14 bytes, static_assert that it is correct.
	// Actually, no static_assert, we do C99
	if (TRAMPOLINE_SIZE != 14)
		xabort();
}

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

static void
check_trampoline_usage(const struct intercept_desc *desc)
{
	if (!desc->uses_trampoline_table)
		return;

	/*
	 * We might actually not have enough space for creating
	 * more trampolines.
	 */

	size_t used = (size_t)(desc->next_trampoline - desc->trampoline_table);

	if (used + TRAMPOLINE_SIZE >= desc->trampoline_table_size)
		xabort();
}

/*
 * create_patch_wrappers - create the custom assembly wrappers
 * around each syscall to be intercepted. Well, actually, the
 * function create_wrapper does that, so perhaps this function
 * deserves a better name.
 * What this function actually does, is figure out how to create
 * a jump instruction in libc ( which bytes to overwrite ).
 * If it successfully finds suitable bytes for hotpatching,
 * then it determines the exact bytes to overwrite, and the exact
 * address for jumping back to libc.
 *
 * This is all based on the information collected by the routine
 * find_syscalls, which does the disassembling, finding jump destinations,
 * finding padding bytes, etc..
 */
void
create_patch_wrappers(struct intercept_desc *desc)
{
	for (unsigned patch_i = 0; patch_i < desc->count; ++patch_i) {
		struct patch_desc *patch = desc->items + patch_i;

		if (patch->padding_addr != NULL) {
			/*
			 * The preferred option it to use a 5 byte relative
			 * jump in a padding space between symbols in libc.
			 * If such padding space is found, a 2 byte short
			 * jump is enough for jumping to it, thus no
			 * instructions other than the syscall
			 * itself need to be overwritten.
			 */
			patch->uses_padding = true;
			patch->uses_prev_ins = false;
			patch->uses_prev_ins_2 = false;
			patch->uses_next_ins = false;
			patch->dst_jmp_patch = patch->padding_addr;

			/*
			 * Return to libc:
			 * just jump to instruction right after the place
			 * where the syscall instruction was originally.
			 */
			patch->return_address =
			    patch->syscall_addr + SYSCALL_INS_SIZE;
			patch->ok = true;

		} else {
			patch->uses_padding = false;

			/*
			 * No padding space is available, so check the
			 * instructions surrounding the syscall instrucion.
			 * If they can be relocated, then they can be
			 * overwritten. Of course some instrucions depend
			 * on the value of the RIP register, these can not
			 * be relocated.
			 */

			patch->uses_prev_ins =
			    !(patch->preceding_ins.has_ip_relative_opr ||
			    patch->preceding_ins.is_call ||
			    patch->preceding_ins.is_jump ||
			    patch->preceding_ins.is_ret ||
			    has_jump(desc, patch->syscall_addr));

			patch->uses_prev_ins_2 = patch->uses_prev_ins &&
			    !(patch->preceding_ins_2.has_ip_relative_opr ||
			    patch->preceding_ins_2.is_call ||
			    patch->preceding_ins_2.is_ret ||
			    has_jump(desc, patch->syscall_addr
				- patch->preceding_ins.length));

			patch->uses_next_ins =
			    !(patch->following_ins.has_ip_relative_opr ||
			    patch->following_ins.is_rel_jump ||
			    patch->following_ins.is_call ||
			    has_jump(desc,
				patch->syscall_addr + SYSCALL_INS_SIZE));

			/*
			 * Count the number of overwritable bytes
			 * in the variable length.
			 * Sum up the bytes that can be overwritten.
			 * The 2 bytes of the syscall instruction can
			 * be overwritten definitely, so length starts
			 * as SYSCALL_INS_SIZE ( 2 bytes ).
			 */
			unsigned length = SYSCALL_INS_SIZE;

			patch->dst_jmp_patch = patch->syscall_addr;

			/*
			 * If the preceding instruction is relocatable,
			 * add its length. Also, the the instruction right
			 * before that.
			 */
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

			/*
			 * If the followin instrucion is relocatable,
			 * add its length. This also affects the return address.
			 * Normally, the library would return to libc after
			 * handling the syscall by jumping to instruction
			 * right after the syscall. But if that instruction
			 * is overwritten, the returning jump must jump to
			 * the instruction after it.
			 */
			if (patch->uses_next_ins) {
				length += patch->following_ins.length;

				/*
				 * Address of the syscall instrucion
				 * plus 2 bytes
				 * plus the length of the following instruction
				 *
				 * adds up to:
				 *
				 * the address of the second instruction after
				 * the syscall.
				 */
				patch->return_address = patch->syscall_addr +
				    SYSCALL_INS_SIZE +
				    patch->following_ins.length;
			} else {
				/*
				 * Address of the syscall instrucion
				 * plus 2 bytes
				 *
				 * adds up to:
				 *
				 * the address of the first instruction after
				 * the syscall ( just like in the case of
				 * using padding bytes ).
				 */
				patch->return_address =
					patch->syscall_addr + SYSCALL_INS_SIZE;
			}

			/*
			 * If the length is at least 5, then a jump instrucion
			 * with a 32 bit displacement can fit.
			 */
			patch->ok = length >= JUMP_INS_SIZE;

			/* Otherwise give up */
			if (length < JUMP_INS_SIZE) {
				char buffer[0x1000];

				int l = sprintf(buffer,
					"unintercepted syscall at: %s 0x%lx\n",
					desc->dlinfo.dli_fname,
					patch->syscall_offset);

				intercept_log(buffer, (size_t)l);
				xabort();
			}
		}

		create_wrapper(patch, desc->c_detination,
			desc->uses_trampoline_table,
			desc->dlinfo.dli_fname);
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
extern unsigned char intercept_asm_wrapper_mov_return_addr_r11_no_syscall;
extern unsigned char intercept_asm_wrapper_mov_return_addr_r11_syscall;
extern unsigned char intercept_asm_wrapper_mov_libpath_r11;
extern unsigned char intercept_asm_wrapper_mov_magic_r11;
extern unsigned char intercept_asm_wrapper_mov_magic_r11_2;
extern unsigned char intercept_asm_wrapper_simd_save_YMM;
extern unsigned char intercept_asm_wrapper_simd_save_YMM_end;
extern unsigned char intercept_asm_wrapper_simd_restore_YMM;
extern unsigned char intercept_asm_wrapper_simd_restore_YMM_end;
extern unsigned char intercept_asm_wrapper_return_and_no_syscall;
extern unsigned char intercept_asm_wrapper_return_and_syscall;
extern unsigned char intercept_asm_wrapper_push_stack_first_return_addr;
extern unsigned char intercept_asm_wrapper_mov_r11_stack_first_return_addr;

extern void magic_routine();
extern void magic_routine_2();

static size_t tmpl_size;
static ptrdiff_t o_prefix;
static ptrdiff_t o_postfix;
static ptrdiff_t o_call;
static ptrdiff_t o_ret_no_syscall;
static ptrdiff_t o_ret_syscall;
static ptrdiff_t o_ret_jump;
static ptrdiff_t o_push_origin;
static ptrdiff_t o_simd_save;
static ptrdiff_t o_simd_restore;
static ptrdiff_t o_mov_return_r11_no_syscall;
static ptrdiff_t o_mov_return_r11_syscall;
static ptrdiff_t o_mov_libpath_r11;
static ptrdiff_t o_mov_magic_r11;
static ptrdiff_t o_mov_magic_r11_2;
static ptrdiff_t o_push_first_return_addr;
static ptrdiff_t o_mov_r11_first_return_addr;
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
	o_ret_no_syscall = &intercept_asm_wrapper_return_and_no_syscall - begin;
	o_ret_syscall = &intercept_asm_wrapper_return_and_syscall - begin;
	o_ret_jump = &intercept_asm_wrapper_return_jump - begin;
	o_push_origin = &intercept_asm_wrapper_push_origin_addr - begin;
	o_simd_save = &intercept_asm_wrapper_simd_save - begin;
	o_simd_restore = &intercept_asm_wrapper_simd_restore - begin;
	o_mov_return_r11_no_syscall =
	    &intercept_asm_wrapper_mov_return_addr_r11_no_syscall - begin;
	o_mov_return_r11_syscall =
	    &intercept_asm_wrapper_mov_return_addr_r11_syscall - begin;
	o_mov_libpath_r11 = &intercept_asm_wrapper_mov_libpath_r11 - begin;
	o_mov_magic_r11 = &intercept_asm_wrapper_mov_magic_r11 - begin;
	o_mov_magic_r11_2 = &intercept_asm_wrapper_mov_magic_r11_2 - begin;
	o_mov_r11_first_return_addr =
	    &intercept_asm_wrapper_mov_r11_stack_first_return_addr - begin;
	o_push_first_return_addr =
	    &intercept_asm_wrapper_push_stack_first_return_addr - begin;
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
create_movabs_r11(unsigned char *code, uint64_t value)
{
	unsigned char *bytes = (unsigned char *)&value;

	code[0] = 0x49; // movabs opcode
	code[1] = 0xbb; // specifiy r11 as destination
	code[2] = bytes[0];
	code[3] = bytes[1];
	code[4] = bytes[2];
	code[5] = bytes[3];
	code[6] = bytes[4];
	code[7] = bytes[5];
	code[8] = bytes[6];
	code[9] = bytes[7];
}

static void
create_wrapper(struct patch_desc *patch, void *dest_routine,
			bool use_absolute_return,
			const char *libpath)
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

	create_movabs_r11(begin + o_mov_return_r11_no_syscall,
	    (uint64_t)(begin + o_ret_no_syscall));

	create_movabs_r11(begin + o_mov_return_r11_syscall,
	    (uint64_t)(begin + o_ret_syscall));

	create_movabs_r11(begin + o_mov_magic_r11,
	    (uint64_t)&magic_routine + 1);

	create_movabs_r11(begin + o_mov_magic_r11_2,
	    (uint64_t)&magic_routine_2 + 1);

#ifndef NDEBUG

	create_movabs_r11(begin + o_mov_r11_first_return_addr,
	    ((uint64_t)patch->syscall_addr) + 2);

	// write a 'push %r11' instruction
	// overwriting the 'subq $0x8, %rsp' instrucion
	begin[o_push_first_return_addr] = 0x41;
	begin[o_push_first_return_addr + 1] = 0x53;
	begin[o_push_first_return_addr + 2] = 0x90;
	begin[o_push_first_return_addr + 3] = 0x90;
	begin[o_push_first_return_addr + 4] = 0x90;
	begin[o_push_first_return_addr + 5] = 0x90;
	begin[o_push_first_return_addr + 6] = 0x90;
	begin[o_push_first_return_addr + 7] = 0x90;

#endif

	create_movabs_r11(begin + o_mov_libpath_r11, (uint64_t)libpath);

	/* Create the jump instrucions returning to the original code */
	if (use_absolute_return)
		create_absolute_jump(begin + o_ret_jump, patch->return_address);
	else
		create_jump(JMP_OPCODE, begin + o_ret_jump,
				patch->return_address);

	/* Create the jump instrucion calling the intended C function */
	create_jump(JMP_OPCODE, begin + o_call, dest_routine);

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

		if (desc->uses_trampoline_table) {
			/*
			 * First jump to the trampoline table, which
			 * should be in a 2 gigabyte range. From there,
			 * jump to the asm_wrapper.
			 */
			check_trampoline_usage(desc);
			create_jump(JMP_OPCODE,
				patch->dst_jmp_patch, desc->next_trampoline);
			create_absolute_jump(
				desc->next_trampoline, patch->asm_wrapper);
			desc->next_trampoline += TRAMPOLINE_SIZE;
		} else {
			create_jump(JMP_OPCODE,
				patch->dst_jmp_patch, patch->asm_wrapper);
		}

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
