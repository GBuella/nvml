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
 * mkfs-pmemfile.c -- pmemfile mkfs command source file
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libpmemfile-core.h"

static void
print_version(void)
{
	puts("mkfs-pmemfile v0");
}

static void
print_usage(FILE *stream, const char *progname)
{
	fprintf(stream, "Usage: %s [-v] [-h] size path\n", progname);
}

static size_t
parse_size(const char *str)
{
	unsigned long long size;
	char *endptr;

	size = strtoull(str, &endptr, 10);
	if (*endptr != 0 || errno != 0)
		return 0;

	return size;
}

int
main(int argc, char *argv[])
{
	int opt;
	size_t size;
	const char *path;

	while ((opt = getopt(argc, argv, "vh")) >= 0) {
		switch (opt) {
		case 'v':
		case 'V':
			print_version();
			return 0;
		case 'h':
		case 'H':
			print_usage(stdout, argv[0]);
			return 0;
		default:
			print_usage(stderr, argv[0]);
			return 2;
		}
	}

	if (optind + 2 > argc) {
		print_usage(stderr, argv[0]);
		return 2;
	}

	size = parse_size(argv[optind++]);

	if (size == 0) {
		puts("Invalid size");
		print_usage(stderr, argv[0]);
		return 2;
	}

	path = argv[optind];

	if (pmemfile_mkfs(path, size, S_IWUSR | S_IRUSR) == NULL) {
		perror("pmemfile_mkfs ");
		return 1;
	}

	return 0;
}
