/*
 * Copyright 2017, Intel Corporation
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
 * pool_hdr_osx.c -- pool header utilities, OSX-specific
 */

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <mach-o/loader.h>

#include "elf_constants.h"
#include "out.h"
#include "pool_hdr.h"

/*
 * util_get_arch_flags -- get architecture identification flags
 *
 * Translating Mach object header constants to ELF specific constants used
 * in pool headers.
 */
int
util_get_arch_flags(struct arch_flags *arch_flags)
{
	Dl_info dlinfo;

	if (!dladdr(util_get_arch_flags, &dlinfo))
		return -1;

	if (dlinfo.dli_fbase == NULL)
		return -1;

	uint32_t magic = ((uint32_t *)(dlinfo.dli_fbase))[0];

	cpu_type_t cputype;

	/* BEGIN CSTYLED */
	/* About Mach-o header specific constants see:
	https://opensource.apple.com/source/xnu/xnu-3248.60.10/EXTERNAL_HEADERS/mach-o/loader.h.auto.html
	*/
	/* END CSTYLED */
	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
		arch_flags->ei_class = ELFCLASS64;
		cputype = ((struct mach_header_64 *)dlinfo.dli_fbase)->cputype;
	} else if (magic == MH_MAGIC || magic == MH_CIGAM) {
		arch_flags->ei_class = ELFCLASS32;
		cputype = ((struct mach_header *)dlinfo.dli_fbase)->cputype;
	} else {
		return -1;
	}

	/* BEGIN CSTYLED */
	/* About CPU type specific constants see:
	https://opensource.apple.com/source/xnu/xnu-3248.60.10/osfmk/mach/machine.h.auto.html
	*/
	/* END CSTYLED */

	if (cputype == CPU_TYPE_X86_64) {
		arch_flags->e_machine = EM_X86_64;
		arch_flags->ei_data = ELFDATA2LSB;
	} else if (cputype == CPU_TYPE_X86) {
		arch_flags->e_machine = EM_386;
		arch_flags->ei_data = ELFDATA2LSB;
	} else {
		arch_flags->e_machine = EM_NONE;
		arch_flags->ei_data = ELFDATANONE;
	}

	arch_flags->alignment_desc = alignment_desc();

	return 0;
}
