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
 * test_libcintercept.c -- dummy program, to issue some syscalls via libc
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

#include <pthread.h>

static void *
busy(void *arg)
{
	FILE *f = (FILE *)arg;
	char buffer[0x100];
	size_t s;

	usleep(100000);
	s = fread(buffer, 1, sizeof(buffer), f);
	usleep(100000);
	fwrite(buffer, 1, s, stdout);
	putchar('\n');
	usleep(100000);
	puts("Done being busy here");
	fflush(stdout);
	usleep(10000);

	return NULL;
}

int
main(int argc, char *argv[])
{
	FILE *f;

	setvbuf(stdout, NULL, _IOLBF, 0);

	if (argc < 2)
		return EXIT_FAILURE;

	if ((f = fopen(argv[1], "r")) == NULL)
		return EXIT_FAILURE;

	if (fork() == 0) {
		busy(f);
	} else {
		wait(NULL);
#ifdef USE_CLONE
		pthread_t t;
		if (pthread_create(&t, NULL, busy, f) != 0)
			return EXIT_FAILURE;
		pthread_join(t, NULL);
#endif
		busy(f);
	}

	fclose(f);

	return EXIT_SUCCESS;
}
