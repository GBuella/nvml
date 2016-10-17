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

#ifndef PMEMFILE_PRELOAD_MOCK_CORELIB_H
#define PMEMFILE_PRELOAD_MOCK_CORELIB_H

#include "libpmemfile-core.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

struct linux_dirent;
struct linux_dirent64;

int mock_pmemfile_chdir(PMEMfilepool *, const char *path);
int mock_pmemfile_lstat(PMEMfilepool *, const char *path, struct stat *buf);
int mock_pmemfile_readlink(PMEMfilepool *, const char *path,
			char *buf, size_t buf_len);
char *mock_pmemfile_getcwd(PMEMfilepool *, char *buf, size_t buf_len);


#define pmemfile_chdir mock_pmemfile_chdir
#define pmemfile_lstat mock_pmemfile_lstat
#define pmemfile_readlink mock_pmemfile_readlink
#define pmemfile_getcwd mock_pmemfile_getcwd

#endif
