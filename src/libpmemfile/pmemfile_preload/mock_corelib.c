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

#include <stddef.h>
#include <errno.h>
#include <syscall.h>
#include <sys/mman.h>

#include "mock_corelib.h"

#include "libcintercept_hook_point.h"

static void
check_pfp(PMEMfilepool *pfp)
{
	if (pfp == NULL) {
		char msg[] = "NULL PMEMfilepool pointer";
		syscall_no_intercept(SYS_write, 2, msg, sizeof(msg));
		syscall_no_intercept(SYS_exit_group, 1);
	}
}

int
mock_pmemfile_chdir(PMEMfilepool *pfp, const char *path)
{
	// Needed during path resolution
	check_pfp(pfp);

	(void) path;

	return 0;
}

int
mock_pmemfile_lstat(PMEMfilepool *pfp, const char *path, struct stat *buf)
{
	// Needed during path resolution
	check_pfp(pfp);

	(void) path;
	(void) buf;

	// TODO: write mock info to *buf
	return 0;
}

int
mock_pmemfile_readlink(PMEMfilepool *pfp, const char *path,
			char *buf, size_t buf_len)
{
	// Needed during path resolution
	check_pfp(pfp);

	(void) path;
	(void) buf_len;

	buf[0] = '/';
	buf[1] = 'a';
	buf[2] = '\0';

	return 2;
}

char *
mock_pmemfile_getcwd(PMEMfilepool *pfp, char *buf, size_t buf_len)
{
	// Needed during path resolution
	check_pfp(pfp);

	(void) buf_len;

	buf[0] = '/';
	buf[1] = '\0';

	return buf;
}

