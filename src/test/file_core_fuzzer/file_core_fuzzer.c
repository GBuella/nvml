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
 * file_core_fuzzer.c -- unit test feeding some bogus values to pmemfile-core
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "unittest.h"
#include "pmemfile_test.h"
#include "util.h"

#include "file.h"

static const long long raw[] = {
	0x0000000000000000ll,
	0x0000000000000001ll,
	0x00000000ffffffffll, /* UINT32_MAX */
	0x0000000080000000ll, /* INT32_MIN */
	0x000000007fffffffll, /* INT32_MAX */
	0x00000000000000ffll, /*  8 bit -1 */
	0xffffffffffffffffll, /* 64 bit -1 */
	0xffffffff00000000ll, /* 32 bit -1 shifted */
	0x0000000100000000ll, /* 2^32 */
	0x0000000000001000ll, /* 4096 */
	0x0000000000001001ll, /* 4096 + 1 */
	0x0000000000000fffll, /* 4096 - 1 */
	0x00000000fffff000ll, /* 32 bit -4096 */
	0x00000000fffff001ll, /* 32 bit -4096 + 1 */
	0x00000000fffffeffll, /* 32 bit -4096 - 1 */
	0xfffffffffffff000ll, /* 64 bit -4096 */
	0xfffffffffffff001ll, /* 64 bit -4096 + 1 */
	0xfffffffffffffeffll, /* 64 bit -4096 - 1 */
	0xdeadbeefdeadbeefll, /* and some dead beefs */
	0x00000000deadbeefll,
	0xdeadbeef00000000ll
};

static const off_t *values_off_t = (const off_t *)&raw;
static const size_t *values_size_t = (const size_t *)&raw;
static const int raw_count = (int)(sizeof(raw) / sizeof(raw[0]));

static void
test1(PMEMfilepool *pfp)
{
	PMEMfile *f = PMEMFILE_OPEN(pfp, "/file1", O_CREAT | O_EXCL | O_WRONLY,
			0644);

	PMEMFILE_LIST_FILES(pfp, "/", (const struct pmemfile_ls[]) {
	    {040777, 2, 4008, "."},
	    {040777, 2, 4008, ".."},
	    {0100644, 1, 0, "file1"},
	    {}});

	static const char *data =
	    "We shall go on to the end. We shall fight in France, we shall "
	    "fight on the seas and oceans, we shall fight with growing "
	    "confidence and growing strength in the air, we shall defend our "
	    "island, whatever the cost may be. We shall fight on the beaches, "
	    "we shall fight on the landing grounds, we shall fight in the "
	    "fields and in the streets, we shall fight in the hills; we shall "
	    "never surrender";

	UT_ASSERTeq(f->offset, 0);
	PMEMFILE_LSEEK(pfp, f, 1, SEEK_SET, 1);
	UT_ASSERTeq(f->offset, 1);
	PMEMFILE_WRITE(pfp, f, data, sizeof(data), sizeof(data));
	UT_ASSERTeq(f->offset, 1 + sizeof(data));

	for (int i = 0; i < raw_count; ++i) {
		off_t off = values_off_t[i];
		off_t expected = off;

		if (off < 0)
			expected = -1;
		errno = 0;

		PMEMFILE_LSEEK(pfp, f, off, SEEK_SET, expected);

		if (off < 0)
			UT_ASSERTeq(errno, EINVAL);
	}

	PMEMFILE_LIST_FILES(pfp, "/", (const struct pmemfile_ls[]) {
	    {040777, 2, 4008, "."},
	    {040777, 2, 4008, ".."},
	    {0100644, 1, 1 + sizeof(data), "file1"},
	    {}});

	PMEMFILE_CLOSE(pfp, f);

}

int
main(int argc, char *argv[])
{
	(void) values_off_t;
	(void) values_size_t;

	START(argc, argv, "file_core_rw");

	if (argc < 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	PMEMfilepool *pfp = PMEMFILE_MKFS(path);

	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 0,
		.blocks = 0});
	PMEMFILE_ASSERT_EMPTY_DIR(pfp, "/");

	test1(pfp);

	pmemfile_pool_close(pfp);

	DONE(NULL);
}
