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

#ifndef LIBSYSCALL_INTERCEPT_HOOK_POINT_H
#define LIBSYSCALL_INTERCEPT_HOOK_POINT_H

/*
 * The inteface for using the intercepting library.
 * This callback function should be implemented by
 * the code using the library.
 *
 * The syscall_number, and the six args describe the syscall
 * currently being intercepted.
 * For now, a non-zero return value means the interceptor executes
 * the original syscall, use its result. A zero return value
 * means the interceptor should not execute the syscall, and
 * use the integer stored to *result as the result of the syscall
 * to be returned in RAX to libc.
 */

extern
#ifdef __cplusplus
"C"
#endif
int (*intercept_hook_point)(long syscall_number,
			long arg0, long arg1,
			long arg2, long arg3,
			long arg4, long arg5,
			long *result);

/*
 * syscall_no_intercept - syscall without interception
 *
 * Call syscall_no_intercept to make syscalls
 * from the interceptor library, once glibc is already patched.
 * Don't use the syscall function from glibc, that
 * would just result in an infinite recursion.
 */
long syscall_no_intercept(long syscall_number, ...);

int libc_hook_in_process_allowed(void);

#endif