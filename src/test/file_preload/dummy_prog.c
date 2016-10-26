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
 * dummy_prog.c - a dummy prog using pmemfile, checking each return value
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
	int fd;
	char buf0[] = "Hello #0 World!\n";
	char buf1[] = "Hello #1 World!\n";

	if (argc < 4)
		return 1;

	const char *full_path = argv[1];
	const char *dir = argv[2];
	const char *relative_path = argv[3];

	if ((fd = open(full_path, O_CREAT | O_RDWR, 0666)) < 0)
		return 1;

	if (close(fd) != 0)
		return 1;

	if ((fd = open(full_path, O_WRONLY, 0)) < 0)
		return 1;

	if (write(fd, buf0, sizeof(buf0)) != sizeof(buf0))
		return 1;

	if (close(fd) != 0)
		return 1;

	if (chdir(dir) != 0)
		return 1;

	if ((fd = open(relative_path, O_CREAT | O_RDWR, 0666)) < 0)
		return 1;

	if (close(fd) != 0)
		return 1;

	if ((fd = open(relative_path, O_WRONLY, 0)) < 0)
		return 1;

	if (write(fd, buf1, sizeof(buf1)) != sizeof(buf1))
		return 1;

	if (close(fd) != 0)
		return 1;

	return 0;
}
