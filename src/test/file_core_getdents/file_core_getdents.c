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
 * file_core_getdents.c -- unit test for pmemfile_getdents & pmemfile_getdents64
 */

#include "unittest.h"
#include <ctype.h>

static PMEMfilepool *
create_pool(const char *path)
{
	PMEMfilepool *pfp = pmemfile_mkfs(path,
			1024 * 1024 * 1024 /* PMEMOBJ_MIN_POOL */,
			S_IWUSR | S_IRUSR);
	if (!pfp)
		UT_FATAL("!pmemfile_mkfs: %s", path);
	return pfp;
}

static void
dump_linux_dirents(void *dirp, unsigned length)
{
	char *buf = (void *)dirp;
	for (int i = 0; i < length; ) {
		long ino = *(long *)&buf[i];
		UT_OUT("d_ino.txt: 0x%016lx", ino);
		UT_OUTF(OF_NONL, "d_ino.bin: ");
		for (int j = 0; j < 8; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		long off = *(long *)&buf[i];
		UT_OUT("d_off.txt: 0x%016lx", off);
		UT_OUTF(OF_NONL, "d_off.bin: ");
		for (int j = 0; j < 8; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		short int reclen = *(short *)&buf[i];
		UT_OUT("d_reclen.txt: %hd", reclen);
		UT_OUTF(OF_NONL, "d_reclen.bin: ");
		for (int j = 0; j < 2; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		UT_OUT("d_name.txt: \"%s\"", buf + i);
		UT_OUTF(OF_NONL, "d_name.bin: ");
		for (int j = 0; j < reclen - 8 - 8 - 2; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx (%c) ",
					buf[i], isprint(buf[i]) ? buf[i] : '?');
		UT_OUTF(OF_NOPREFIX, " ");
		UT_OUT("-");
	}
	UT_OUT("---");
}

static void
dump_linux_dirents64(void *dirp, unsigned length)
{
	char *buf = (void *)dirp;
	for (int i = 0; i < length; ) {
		long ino = *(long *)&buf[i];
		UT_OUT("d_ino.txt: 0x%016lx", ino);
		UT_OUTF(OF_NONL, "d_ino.bin: ");
		for (int j = 0; j < 8; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		long off = *(long *)&buf[i];
		UT_OUT("d_off.txt: 0x%016lx", off);
		UT_OUTF(OF_NONL, "d_off.bin: ");
		for (int j = 0; j < 8; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		short int reclen = *(short *)&buf[i];
		UT_OUT("d_reclen.txt: %hd", reclen);
		UT_OUTF(OF_NONL, "d_reclen.bin: ");
		for (int j = 0; j < 2; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		char type = *(char *)&buf[i];
		UT_OUT("d_type.txt: %hd", type);
		UT_OUTF(OF_NONL, "d_type.bin: ");
		for (int j = 0; j < 1; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx ", buf[i]);
		UT_OUTF(OF_NOPREFIX, " ");

		UT_OUT("d_name.txt: \"%s\"", buf + i);
		UT_OUTF(OF_NONL, "d_name.bin: ");
		for (int j = 0; j < reclen - 8 - 8 - 2 - 1; ++j, ++i)
			UT_OUTF(OF_NONL | OF_NOPREFIX, "0x%02hhx (%c) ",
					buf[i], isprint(buf[i]) ? buf[i] : '?');
		UT_OUTF(OF_NOPREFIX, " ");
		UT_OUT("-");
	}

	UT_OUT("---");
}

static void
test1(PMEMfilepool *pfp)
{
	PMEMfile *f = pmemfile_open(pfp, "/file1", O_CREAT | O_EXCL | O_WRONLY,
			0644);
	UT_ASSERTne(f, NULL);
	pmemfile_close(pfp, f);

	f = pmemfile_open(pfp, "/file2with_long_name",
			O_CREAT | O_EXCL | O_WRONLY,
			0644);
	UT_ASSERTne(f, NULL);
	pmemfile_close(pfp, f);

	f = pmemfile_open(pfp, "/file3with_very_long_name"
			"_1234567890_1234567890_1234567890_1234567890"
			"_1234567890_1234567890_1234567890_1234567890"
			"_1234567890_1234567890_1234567890_1234567890"
			"_1234567890_1234567890_1234567890_1234567890"
			"_1234567890_1234567890_1234567890_1234567890"
			"_qwertyuiop",
			O_CREAT | O_EXCL | O_WRONLY,
			0644);
	UT_ASSERTne(f, NULL);
	pmemfile_close(pfp, f);


	f = pmemfile_open(pfp, "/file4", O_CREAT | O_EXCL | O_WRONLY,
			0644);
	UT_ASSERTne(f, NULL);
	pmemfile_close(pfp, f);

	f = pmemfile_open(pfp, "/", O_DIRECTORY | O_RDONLY);
	UT_ASSERTne(f, NULL);

	char buf[32758];
	int r = pmemfile_getdents(pfp, f, (void *)buf, sizeof(buf));
	UT_ASSERT(r > 0);

	dump_linux_dirents(buf, r);

	r = pmemfile_getdents(pfp, f, (void *)buf, sizeof(buf));
	UT_ASSERT(r == 0);

	r = pmemfile_lseek(pfp, f, 0, SEEK_SET);
	UT_ASSERT(r == 0);

	r = pmemfile_getdents64(pfp, f, (void *)buf, sizeof(buf));
	UT_ASSERT(r > 0);

	dump_linux_dirents64(buf, r);

	r = pmemfile_getdents64(pfp, f, (void *)buf, sizeof(buf));
	UT_ASSERT(r == 0);

	pmemfile_close(pfp, f);
}

int
main(int argc, char *argv[])
{
	START(argc, argv, "file_core_getdents");

	if (argc < 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	PMEMfilepool *pfp = create_pool(path);

	test1(pfp);

	pmemfile_pool_close(pfp);

	DONE(NULL);
}