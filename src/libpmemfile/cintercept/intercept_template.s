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

/*
 * intercept_template.s
 *
 * The syscall instructions in glbic are
 * overwritten with a call instruction, which
 * jumps here. This assembly wrapper has two jobs:
 * Make sure none of the registers are clobbered
 * from the caller's point of view, and convert
 * between calling conventions -- the syscall
 * arguments are expected to be set up already
 * at the point of call.
 *
 * This code is a template, it is going to copied,
 * and modified to fit particularities of each call.
 * The prefix, postfix labels are there to make it
 * easy to find the placeholder nops.
 *
 * Excerpts from http://wiki.osdev.org/System_V_ABI#x86-64 :
 * "Functions preserve the registers rbx, rsp, rbp, r12, r13, r14, and r15;
 * while rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11 are scratch registers."
 * "The stack is 16-byte aligned just before the call instruction is called."
 */

.global xlongjmp;
.type   xlongjmp, @function

.global magic_routine;
.type   magic_routine, @function

.global intercept_asm_wrapper_tmpl;
.global intercept_asm_wrapper_simd_save;
.global intercept_asm_wrapper_prefix;
.global intercept_asm_wrapper_push_origin_addr;
.global intercept_asm_wrapper_mov_return_addr_r11;
.global intercept_asm_wrapper_mov_libpath_r11;
.global intercept_asm_wrapper_mov_magic_r11;
.global intercept_asm_wrapper_call;
.global intercept_asm_wrapper_simd_restore;
.global intercept_asm_wrapper_postfix;
.global intercept_asm_wrapper_return_jump;
.global intercept_asm_wrapper_end;
.global intercept_asm_wrapper_simd_save_YMM;
.global intercept_asm_wrapper_simd_save_YMM_end;
.global intercept_asm_wrapper_simd_restore_YMM;
.global intercept_asm_wrapper_simd_restore_YMM_end;


.text

xlongjmp:
	.cfi_startproc
	mov         %rdx, %rax
	mov         %rsi, %rsp
	jmp         *%rdi
	.cfi_endproc

.size   xlongjmp, .-xlongjmp

magic_routine:
	.cfi_startproc
	.cfi_def_cfa_offset 0x560
.fill 2, 1, 0x90
	.cfi_endproc

.size   magic_routine, .-magic_routine


intercept_asm_wrapper_tmpl:
	nop

intercept_asm_wrapper_prefix:
	/*
	 * The placeholder nops for whatever instruction
	 * preceding the syscall instruction in glibc was overwritten
	 */
.fill 20, 1, 0x90

	/*
	 * Jump back to libc on clone, vfork, execve, rt_sigreturn.
	 */ 
	cmp         $0x38, %rax
	je          L1
	cmp         $0x3a, %rax
	je          L1
	cmp         $0x3b, %rax
	je          L1
	cmp         $0x0f, %rax
	je          L1

	subq        $0x80, %rsp  /* red zone */

	pushq       %rbp
	movq        %rsp, %rbp /* save the original rsp value */
	addq        $0x88, %rbp
	pushf
	pushq       %r15
	pushq       %r14
	pushq       %r13
	pushq       %r12
	pushq       %r10
	pushq       %r9
	pushq       %r8
	pushq       %rcx
	pushq       %rdx
	pushq       %rsi
	pushq       %rdi

	pushq       %rbx

	movq        %rsp, %rbx

	orq         $0x1f, %rsp
	subq        $0x3f, %rsp
	/*
	 * Reserve stack for SIMD registers.
	 * Largest space is used in the AVX512 case, 32 * 32 bytes.
	 */
	subq       $0x400, %rsp
intercept_asm_wrapper_simd_save:
	/*
	 * Save any SIMD registers that need to be saved,
	 * these nops are going to be replace with CPU
	 * dependent code.
	 */
	movaps      %xmm0, (%rsp)
	movaps      %xmm1, 0x10 (%rsp)
	movaps      %xmm2, 0x20 (%rsp)
	movaps      %xmm3, 0x30 (%rsp)
	movaps      %xmm4, 0x40 (%rsp)
	movaps      %xmm5, 0x50 (%rsp)
	movaps      %xmm6, 0x60 (%rsp)
	movaps      %xmm7, 0x70 (%rsp)
.fill 32, 1, 0x90

	pushq       %rbx
	movq        %rsp, %r11

	movq        %rbp, %rsp
	subq        $0x530, %rsp

	pushq       %r11

intercept_asm_wrapper_mov_return_addr_r11:
	/*
	 * Placeholder for a movabs instruction, putting the return address
	 * into r11. This should help unwinding the stack.
	 */
.fill 10, 1, 0x90
	pushq       %r11

intercept_asm_wrapper_mov_libpath_r11:
.fill 10, 1, 0x90
	pushq       %r11

intercept_asm_wrapper_push_origin_addr:
.fill 5, 1, 0x90


	/*
	 * Convert the arguments list to one used in
	 * the linux x86_64 ABI. The reverse of what
	 * is done syscall_no_intercept.
	 *
	 * syscall arguments are expected in:
	 *   rax, rdi, rsi, rdx, r10, r8, r9
	 *
	 * C function expects arguments in:
	 *   rdi, rsi, rdx, rcx, r8, r9, [rsp + 8]
	 */
	pushq       %r9

	movq        %r8, %r9
	movq        %r10, %r8
	movq        %rdx, %rcx
	movq        %rsi, %rdx
	movq        %rdi, %rsi
	movq        %rax, %rdi

intercept_asm_wrapper_mov_magic_r11:
.fill 10, 1, 0x90
	pushq       %r11  /* push the fake return address */

intercept_asm_wrapper_call:
	/*
	 * Calling into the code written in C.
	 * Use the return value in rax as the return value
	 * of the syscall.
	 */
.fill 5, 1, 0x90

	/* addq        $0x18, %rsp */

	popq        %rbx

intercept_asm_wrapper_simd_restore:
	movaps      (%rsp), %xmm0
	movaps      0x10 (%rsp), %xmm1
	movaps      0x20 (%rsp), %xmm2
	movaps      0x30 (%rsp), %xmm3
	movaps      0x40 (%rsp), %xmm4
	movaps      0x50 (%rsp), %xmm5
	movaps      0x60 (%rsp), %xmm6
	movaps      0x70 (%rsp), %xmm7
.fill 32, 1, 0x90


	movq        %rbx, %rsp

	popq        %rbx    /* restoring the rest of the registers */

	popq        %rdi
	popq        %rsi
	popq        %rdx
	popq        %rcx
	popq        %r8
	popq        %r9
	popq        %r10
	popq        %r12
	popq        %r13
	popq        %r14
	popq        %r15
	popf
	popq        %rbp
	addq        $0x80, %rsp  /* return address + mock rbp + red zone */

	jmp         L2
L1:
	syscall
L2:
	nop
intercept_asm_wrapper_postfix:
	/*
	 * The placeholder nops for whatever instruction
	 * following the syscall instruction in glibc was overwritten.
	 */
.fill 20, 1, 0x90

intercept_asm_wrapper_return_jump:
.fill 20, 1, 0x90

intercept_asm_wrapper_end:

intercept_asm_wrapper_simd_save_YMM:
	vmovaps     %ymm0, (%rsp)
	vmovaps     %ymm1, 0x20 (%rsp)
	vmovaps     %ymm2, 0x40 (%rsp)
	vmovaps     %ymm3, 0x60 (%rsp)
	vmovaps     %ymm4, 0x80 (%rsp)
	vmovaps     %ymm5, 0xa0 (%rsp)
	vmovaps     %ymm6, 0xc0 (%rsp)
	vmovaps     %ymm7, 0xe0 (%rsp)
intercept_asm_wrapper_simd_save_YMM_end:

intercept_asm_wrapper_simd_restore_YMM:
	vmovaps     (%rsp), %ymm0
	vmovaps     0x20 (%rsp), %ymm1
	vmovaps     0x40 (%rsp), %ymm2
	vmovaps     0x60 (%rsp), %ymm3
	vmovaps     0x80 (%rsp), %ymm4
	vmovaps     0xa0 (%rsp), %ymm5
	vmovaps     0xc0 (%rsp), %ymm6
	vmovaps     0xe0 (%rsp), %ymm7
intercept_asm_wrapper_simd_restore_YMM_end:
