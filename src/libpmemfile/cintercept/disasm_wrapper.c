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
 * disasm_wrapper.c -- connecting the interceptor code
 * to the disassembler code from the nasm project.
 */

#include "intercept.h"
#include "intercept_util.h"
#include "disasm_wrapper.h"

#include "disasm.h"

#define SEG_RMREG 4

struct intercept_disasm_context *
intercept_disasm_init(const unsigned char *begin, const unsigned char *end)
{
	(void) begin;
	(void) end;

	return NULL; // nothing to do for NASM
}

void
intercept_disasm_destroy(struct intercept_disasm_context *context)
{
	(void) context; // nothing to do for NASM
}

struct intercept_disasm_result
intercept_disasm_next_instruction(struct intercept_disasm_context *context,
					const unsigned char *code)
{
	(void) context;

	struct intercept_disasm_result result;
	iflag_t prefer;
	iflag_clear_all(&prefer);
	struct insn instruction;
	const struct itemplate *instruction_template;

	result.length = disasm(code, 64, &prefer,
				&instruction, &instruction_template);

	if (result.length == 0)
		return result;

/*
 * replicate the way nasm decides about an operand being IP relative
 * I don't fully understand this code right now.
 */

	result.has_ip_relative_opr = false;

	for (int i = 0; i < instruction_template->operands; ++i) {
		opflags_t t = instruction_template->opd[i];

		if (((t & (REGISTER | FPUREG)) ||
		    (instruction.oprs[i].segment & SEG_RMREG)) ||
		    (!(UNITY & ~t)) ||
		    (t & IMMEDIATE) ||
		    (!(MEM_OFFS & ~t)))
				continue;

		if (is_class(REGMEM, instruction_template->opd[i])) {
			if (instruction.oprs[i].eaflags & EAF_REL)
				result.has_ip_relative_opr = true;
		}
	}

	result.is_syscall = instruction_template->opcode == I_SYSCALL;

	result.is_call = instruction_template->opcode == I_CALL;

	result.is_ret = instruction_template->opcode == I_RET;

	result.is_rel_jump =
		instruction_template->opcode == I_JMP ||
		instruction_template->opcode == I_JECXZ ||
		instruction_template->opcode == I_JCXZ ||
		instruction_template->opcode == I_JMPE ||
		instruction_template->opcode == I_JRCXZ ||
		instruction_template->opcode == I_Jcc ||
		instruction_template->opcode == I_CALL;

	if (result.is_rel_jump) {
		result.jump_delta = instruction.oprs[0].offset;
		result.jump_target =
		    code + result.length + result.jump_delta;
	}

	return result;
}
