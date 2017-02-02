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
 * file_core_symlinks.c -- unit test for pmemfile_symlink and co
 */
#define _GNU_SOURCE
#include "unittest.h"
#include "pmemfile_test.h"

static PMEMfilepool *
create_pool(const char *path)
{
	PMEMfilepool *pfp = pmemfile_mkfs(path, PMEMOBJ_MIN_POOL,
			S_IWUSR | S_IRUSR);
	if (!pfp)
		UT_FATAL("!pmemfile_mkfs: %s", path);
	return pfp;
}

static PMEMfilepool *
open_pool(const char *path)
{
	PMEMfilepool *pfp = pmemfile_pool_open(path);
	if (!pfp)
		UT_FATAL("!pmemfile_pool_open %s", path);
	return pfp;
}

static void
test0(PMEMfilepool *pfp)
{
	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 0,
		.blocks = 0});

	PMEMFILE_ASSERT_EMPTY_DIR(pfp, "/");

	PMEMFILE_CREATE(pfp, "/file1", 0, 0644);

	PMEMFILE_MKDIR(pfp, "/dir", 0755);

	PMEMFILE_SYMLINK(pfp, "/file1", "/dir/sym1-exists");
	PMEMFILE_READLINK(pfp, "/dir/sym1-exists", "/file1");
	PMEMFILE_READLINKAT(pfp, "/dir", "sym1-exists", "/file1");
	PMEMFILE_READLINKAT(pfp, "/", "dir/sym1-exists", "/file1");

	PMEMFILE_SYMLINK(pfp, "/file2", "/dir/sym2-not_exists");
	PMEMFILE_READLINK(pfp, "/dir/sym2-not_exists", "/file2");
	PMEMFILE_READLINKAT(pfp, "/dir", "sym2-not_exists", "/file2");

	PMEMFILE_SYMLINK(pfp, "../file1", "/dir/sym3-exists-relative");
	PMEMFILE_READLINK(pfp, "/dir/sym3-exists-relative", "../file1");
	PMEMFILE_READLINKAT(pfp, "/dir", "sym3-exists-relative", "../file1");

	PMEMFILE_SYMLINK(pfp, "../file2", "/dir/sym4-not_exists-relative");
	PMEMFILE_READLINK(pfp, "/dir/sym4-not_exists-relative", "../file2");
	PMEMFILE_READLINKAT(pfp, "/dir", "sym4-not_exists-relative",
			"../file2");

	PMEMFILE_LIST_FILES(pfp, "/", (const struct pmemfile_ls[]) {
	    {040777, 3, 4008, "."},
	    {040777, 3, 4008, ".."},
	    {0100644, 1, 0, "file1"},
	    {040755, 2, 4008, "dir"},
	    {}});

	PMEMFILE_LIST_FILES(pfp, "/dir", (const struct pmemfile_ls[]) {
	    {040755, 2, 4008, "."},
	    {040777, 3, 4008, ".."},
	    {0120777, 1, 6, "sym1-exists", "/file1"},
	    {0120777, 1, 6, "sym2-not_exists", "/file2"},
	    {0120777, 1, 8, "sym3-exists-relative", "../file1"},
	    {0120777, 1, 8, "sym4-not_exists-relative", "../file2"},
	    {}});

	int ret;

	ret = pmemfile_symlink(pfp, "whatever", "/not-exisiting-dir/xxx");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOENT);

	ret = pmemfile_symlink(pfp, "whatever", "/file1/xxx");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOTDIR);

	ret = pmemfile_symlink(pfp, "whatever", "/dir/sym1-exists");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, EEXIST);

	char tmp[4096];
	memset(tmp, '0', 4095);
	tmp[4095] = 0;

	ret = pmemfile_symlink(pfp, tmp, "/dir/lalala");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENAMETOOLONG);

	PMEMFILE_MKDIR(pfp, "/deleted-dir", 0755);
	PMEMfile *deleted_dir = PMEMFILE_OPEN(pfp, "/deleted-dir", O_DIRECTORY);
	PMEMFILE_RMDIR(pfp, "/deleted-dir");

	ret = pmemfile_symlinkat(pfp, "whatever", deleted_dir, "lalala");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOENT);
	PMEMFILE_CLOSE(pfp, deleted_dir);

	PMEMfile *f = PMEMFILE_OPEN(pfp, "/file1", O_RDONLY);
	ret = pmemfile_symlinkat(pfp, "whatever", f, "lalala");
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOTDIR);


	char buf[PATH_MAX];

	ret = pmemfile_readlink(pfp, "/not-existing-dir/xxx", buf, PATH_MAX);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOENT);

	ret = pmemfile_readlink(pfp, "/file1/xxx", buf, PATH_MAX);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOTDIR);

	ret = pmemfile_readlink(pfp, "/file1", buf, PATH_MAX);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, EINVAL);

	ret = pmemfile_readlinkat(pfp, f, "lalala", buf, PATH_MAX);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOTDIR);

	ret = pmemfile_readlink(pfp, "/dir/sym1-exists/", buf, PATH_MAX);
	UT_ASSERTeq(ret, -1);
	UT_ASSERTeq(errno, ENOTDIR);

	PMEMFILE_CLOSE(pfp, f);

	PMEMFILE_UNLINK(pfp, "/dir/sym1-exists");
	PMEMFILE_UNLINK(pfp, "/dir/sym2-not_exists");
	PMEMFILE_UNLINK(pfp, "/dir/sym3-exists-relative");
	PMEMFILE_UNLINK(pfp, "/dir/sym4-not_exists-relative");
	PMEMFILE_UNLINK(pfp, "/file1");
	PMEMFILE_RMDIR(pfp, "/dir");

	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 1,
		.blocks = 0});

	pmemfile_pool_close(pfp);
}

static void
test_symlink_valid(PMEMfilepool *pfp, const char *path)
{
	char buf[4096];

	memset(buf, 0, sizeof(buf));
	PMEMfile *file = PMEMFILE_OPEN(pfp, path, O_RDONLY);
	PMEMFILE_READ(pfp, file, buf, sizeof(buf), 7);
	PMEMFILE_CLOSE(pfp, file);
	UT_ASSERTeq(memcmp(buf, "qwerty\n", 7), 0);
}

static void
test_symlink_to_dir_valid(PMEMfilepool *pfp, const char *path)
{
	char buf[1];
	PMEMfile *file = pmemfile_open(pfp, path, O_RDONLY | O_NOFOLLOW);
	UT_ASSERTeq(file, NULL);
	UT_ASSERTeq(errno, ELOOP);

	file = PMEMFILE_OPEN(pfp, path, O_RDONLY | O_NOFOLLOW | O_PATH);
	PMEMFILE_READ(pfp, file, buf, sizeof(buf), -1, EBADF);
	PMEMFILE_CLOSE(pfp, file);
}

static void
test_symlink_invalid(PMEMfilepool *pfp, const char *path)
{
	PMEMfile *file = pmemfile_open(pfp, path, O_RDONLY);
	UT_ASSERTeq(file, NULL);
	UT_ASSERTeq(errno, ENOENT);
}

static void
test1(PMEMfilepool *pfp)
{
	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 1,
		.blocks = 0});

	PMEMFILE_ASSERT_EMPTY_DIR(pfp, "/");

	PMEMFILE_MKDIR(pfp, "/dir1", 0755);
	PMEMFILE_MKDIR(pfp, "/dir1/internal_dir", 0755);
	PMEMFILE_MKDIR(pfp, "/dir2", 0755);

	PMEMFILE_SYMLINK(pfp, "/dir1/internal_dir", "/dir2/symlink_dir1");
	PMEMFILE_SYMLINK(pfp, "../dir1/internal_dir", "/dir2/symlink_dir2");

	PMEMFILE_SYMLINK(pfp, "/dir1/not_existing_dir", "/dir2/symlink_dir3");
	PMEMFILE_SYMLINK(pfp, "../not_existing_dir", "/dir2/symlink_dir4");

	PMEMFILE_SYMLINK(pfp, "/dir2/symlink_dir1", "/symlink_to_symlink_dir");

	PMEMFILE_SYMLINK(pfp, "/dir1", "/dir2/symlink_dir1/dir1");
	PMEMFILE_SYMLINK(pfp, "/dir1/", "/dir2/symlink_dir1/dir1slash");

	PMEMfile *file = PMEMFILE_OPEN(pfp, "/dir1/internal_dir/file",
			O_CREAT | O_WRONLY, 0644);
	PMEMFILE_WRITE(pfp, file, "qwerty\n", 7, 7);
	PMEMFILE_CLOSE(pfp, file);

	test_symlink_valid(pfp, "/dir2/symlink_dir1/file");
	test_symlink_valid(pfp, "/dir2/symlink_dir2/file");
	test_symlink_valid(pfp, "/symlink_to_symlink_dir/file");

	test_symlink_to_dir_valid(pfp, "/dir2/symlink_dir1/dir1");
	test_symlink_to_dir_valid(pfp, "/dir2/symlink_dir1/dir1slash");

	test_symlink_invalid(pfp, "/dir2/symlink_dir3/file");
	test_symlink_invalid(pfp, "/dir2/symlink_dir4/file");

	PMEMFILE_UNLINK(pfp, "/symlink_to_symlink_dir");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir1/dir1");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir1/dir1slash");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir4");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir3");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir2");
	PMEMFILE_UNLINK(pfp, "/dir2/symlink_dir1");
	PMEMFILE_UNLINK(pfp, "/dir1/internal_dir/file");
	PMEMFILE_RMDIR(pfp, "/dir2");
	PMEMFILE_RMDIR(pfp, "/dir1/internal_dir");
	PMEMFILE_RMDIR(pfp, "/dir1");

	PMEMFILE_LIST_FILES(pfp, "/", (const struct pmemfile_ls[]) {
		    {040777, 2, 4008, "."},
		    {040777, 2, 4008, ".."},
		    {}});

	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 1,
		.blocks = 0});

	pmemfile_pool_close(pfp);
}

static void
test2(PMEMfilepool *pfp)
{
	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 1,
		.blocks = 0});

	PMEMFILE_ASSERT_EMPTY_DIR(pfp, "/");

	PMEMfile *file = PMEMFILE_OPEN(pfp, "/file1", O_CREAT | O_WRONLY, 0644);
	PMEMFILE_WRITE(pfp, file, "qwerty\n", 7, 7);
	PMEMFILE_CLOSE(pfp, file);

	PMEMFILE_MKDIR(pfp, "/dir", 0755);

	PMEMFILE_SYMLINK(pfp, "/file1", "/dir/sym1-exists");
	PMEMFILE_SYMLINK(pfp, "/file2", "/dir/sym2-not_exists");
	PMEMFILE_SYMLINK(pfp, "../file1", "/dir/sym3-exists-relative");
	PMEMFILE_SYMLINK(pfp, "../file2", "/dir/sym4-not_exists-relative");

	char buf[4096];

	memset(buf, 0, sizeof(buf));
	file = PMEMFILE_OPEN(pfp, "/file1", O_RDONLY);
	PMEMFILE_READ(pfp, file, buf, sizeof(buf), 7);
	PMEMFILE_CLOSE(pfp, file);
	UT_ASSERTeq(memcmp(buf, "qwerty\n", 7), 0);

	test_symlink_valid(pfp, "/dir/sym1-exists");
	test_symlink_invalid(pfp, "/dir/sym2-not_exists");

	test_symlink_valid(pfp, "/dir/sym3-exists-relative");
	test_symlink_invalid(pfp, "/dir/sym4-not_exists-relative");

	PMEMFILE_UNLINK(pfp, "/dir/sym1-exists");
	PMEMFILE_UNLINK(pfp, "/dir/sym2-not_exists");
	PMEMFILE_UNLINK(pfp, "/dir/sym3-exists-relative");
	PMEMFILE_UNLINK(pfp, "/dir/sym4-not_exists-relative");
	PMEMFILE_UNLINK(pfp, "/file1");
	PMEMFILE_RMDIR(pfp, "/dir");

	PMEMFILE_STATS(pfp, (const struct pmemfile_stats) {
		.inodes = 1,
		.dirs = 0,
		.block_arrays = 0,
		.inode_arrays = 1,
		.blocks = 0});

	pmemfile_pool_close(pfp);
}

int
main(int argc, char *argv[])
{
	START(argc, argv, "file_core_symlinks");

	if (argc < 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	test0(create_pool(path));
	test1(open_pool(path));
	test2(open_pool(path));

	DONE(NULL);
}
