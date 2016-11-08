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

#ifndef PMEMFILE_TEST_H
#define PMEMFILE_TEST_H

#include "libpmemfile-core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pmemfile stuff */
PMEMfilepool *PMEMFILE_MKFS(const char *path);
PMEMfile *PMEMFILE_OPEN(PMEMfilepool *pfp, const char *path, int flags, ...);
ssize_t PMEMFILE_WRITE(PMEMfilepool *pfp, PMEMfile *file, const void *buf,
		size_t count, ssize_t expected, ...);
void PMEMFILE_CLOSE(PMEMfilepool *pfp, PMEMfile *file);
void PMEMFILE_UNLINK(PMEMfilepool *pfp, const char *path);
ssize_t PMEMFILE_READ(PMEMfilepool *pfp, PMEMfile *file, void *buf,
		size_t count, ssize_t expected, ...);
off_t PMEMFILE_LSEEK(PMEMfilepool *pfp, PMEMfile *file, off_t offset,
		int whence, off_t expected);
ssize_t PMEMFILE_FILE_SIZE(PMEMfilepool *pfp, PMEMfile *file,
		ssize_t expected_size);
ssize_t PMEMFILE_PATH_SIZE(PMEMfilepool *pfp, const char *path,
		ssize_t expected_size);

#ifdef __cplusplus
}
#endif

#endif