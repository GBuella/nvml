[comment]: <> (Copyright 2016, Intel Corporation)

[comment]: <> (Redistribution and use in source and binary forms, with or without)
[comment]: <> (modification, are permitted provided that the following conditions)
[comment]: <> (are met:)
[comment]: <> (    * Redistributions of source code must retain the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer.)
[comment]: <> (    * Redistributions in binary form must reproduce the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer in)
[comment]: <> (      the documentation and/or other materials provided with the)
[comment]: <> (      distribution.)
[comment]: <> (    * Neither the name of the copyright holder nor the names of its)
[comment]: <> (      contributors may be used to endorse or promote products derived)
[comment]: <> (      from this software without specific prior written permission.)

[comment]: <> (THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS)
[comment]: <> ("AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR)
[comment]: <> (A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT)
[comment]: <> (OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,)
[comment]: <> (SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,)
[comment]: <> (DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY)
[comment]: <> (THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT)
[comment]: <> ((INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE)
[comment]: <> (OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.)

[comment]: <> (libcintercept.3 -- man page for libcintercept)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[EXAMPLE](#example)<br />
[UNDER THE HOOD](#under-the-hood)<br />
[LIMITATIONS](#limitations)<br />


# NAME #

**libcintercept** -- Syscall intercepting library

# SYNOPSIS #

```c
#include <libcintercept_hook_point.h>
```
```sh
cc -lcintercept -fpic -shared
```

##### Description: #####

The system call intercepting library provides a low-level interface
for hooking Linux system calls in user space. This is achieved
by hotpatching the machine code of the standard C library in the
memory of a process. The user of this library can provide the
functionality of almost any syscall in user space, using the very
simple API spcified in the libcintercept\_hook\_point.h header file:
```c
int (*intercept_hook_point)(long syscall_number,
			long arg0, long arg1,
			long arg2, long arg3,
			long arg4, long arg5,
			long *result);
```

The user of the library shall assgin to the variable called
intercept_hook_point a pointer the address of a callback function.
A non-zero return value returned by the callback function is used
to signal to the intercepting library, that the specific system
call was ignored by the user, and the original syscall should be
executed. A zero return value signals that the user takes over the
system call. In this case, the result of the system call
( the value stored in the RAX register after the system call )
can be set via the *result pointer. In order to use the library,
the intercepting code is expected to be loaded using the
LD_PRELOAD feature provided by the systems loader.

All syscalls issued by libc are intercepted. In order to
be able to issue syscalls that are not intercepted, a
convenience function is provided by the library:
```c
long syscall_no_intercept(long syscall_number, ...);
```

Three environment variables control the operation of the library:

*INTERCEPT_LOG* -- when set, the library logs each syscall
intercepted to a file. The path of the file is formed by appending
a period and a process id to the value provided in the environment
variable. E.g.: initializing the library in a process with pid 123
when the INTERCEPT_LOG=inc.log is set, will result in a log
file named inc.log.123

*INTERCEPT_LOG_NOPID* -- when set, the path of the log
file is taken as is from the INTERCEPT_LOG variable,
i.e.: without appending the process id to it.

*LIBC_HOOK_CMDLINE_FILTER* -- when set, the library
checks the contents of the /proc/self/cmdline file.
Hotpatching, and syscall intercepting is only done, if the
last component of the first zero terminated string in
/proc/self/cmdline matches the string provided
in the environment variable. This can also be queried
by the user of the library:
```c
int libc_hook_in_process_allowed(void);
```

##### Example: #####

```c
#include <libcintercept_hook_point.h>
#include <syscall.h>
#include <errno.h>

static int
hook(long syscall_number,
			long arg0, long arg1,
			long arg2, long arg3,
			long arg4, long arg5,
			long *result)
{
	if (syscall_number == SYS_getdents) {
		/*
		 * Prevent the application from
		 * using the getdents syscall. From
		 * the point of view of the calling
		 * process, it is as if the kernel
		 * would return the ENOTSUP error
		 * code from the syscall.
		 */
		*result = -ENOTSUP;
		return 0;
	} else {
		/*
		 * Ignore any other syscalls
		 * i.e.: pass them on to the kernel
		 * as would normally happen.
		 */
		return 1;
	}
}

static __attribute__((constructor)) void
init(void)
{
	// Set up the callback function
	intercept_hook_point = hook;
}
```

```sh
$ cc example.c -lcintercept -fpic -shared -o example.so
$ LD_LIBRARY_PATH=. LD_PRELOAD=example.so ls
ls: reading directory '.': Operation not supported
```

# Under the hood: #

##### Assumptions: #####
In order to handle syscalls in user space, the library relies
on the following assumptions:

- Each syscall made by the applicaton is issued via libc
- No other facility attempts to hotpach libc in the same process
- The libc implementation is already loaded in the processes
memory space when the intercepting library is being initialized
- The machine code in the libc implementation is suitable
for the methods listed in this section
- For some more basic assumptions, see the section on limitations.

##### Disassembly: #####
The library disassembles the text segment of the libc loaded
into the memory space of the process it is initialized in. It
locates all syscall instructions, and replaces each of them
with a jump to a unique address. Since the syscall instruction
of the x86_64 ISA occupies only two bytes, the method involves
locating other bytes close to the syscall suitable for overwriting.
The destination of the jump ( unique for each syscall ) is a
small routine, which accomplisesh the following tasks:

1. Optionally executes any instruction that originally
preceded the syscall instruction, and was overwritten to
make space for the jump instruction
2. Saves the current state of all registers to the stack
3. Translates the arguments ( in the registers ) from
the Linux x86_64 syscall calling convention to the C ABI's
calling convention used on x86_64
4. Calls a function written in C ( which in turn calls
the callback supplied by the libraries user )
5. Loads the values from the stack back into the registers
6. Jumps back to libc, to the instruction following the
overwritten part

##### In action: #####

*Simple hotpatching:*
Replace a mov and a syscall instruction with a jmp instruction
```
Before:                         After:

db2a0 <__open>:                 db2b0 <__open>:
db2aa: mov $2, %eax           /-db2aa: jmp e0000
db2af: syscall                |
db2b1: cmp $-4095, %rax       | db2b1: cmp $-4095, %rax ---\
db2b7: jae db2ea              | db2b7: jae db2ea           |
db2b9: retq                   | db2b9: retq                |
                              | ...                        |
                              | ...                        |
                              \_...                        |
                                e0000: mov $2, $eax        |
                                ...                        |
                                e0100: call implementation /
                                ...                       /
                                e0200: jmp db2aa ________/
```
*Hotpatching using a trampoline jump:*
Replace a syscall instruction with a short jmp instruction,
the destination of which if is a regular jmp instruction. 
The reason to use this, is that a short jmp instruction
consumes only two bytes, thus fits in the place of a syscall
instruction. Sometimes the instructions directly preceding
or following the syscall instruction can not be overwritten,
leaving only the two bytes of the syscall instruction
for patching.
The hotpatching library looks for place for the trampoline jump
in the padding found to the end of each routine. Since the start
of all routines is aligned to 16 bytes, often there is a padding
space between the end of a symbol, and the start of the next symbol.
In the example below, this padding is filled with 7 byte long
nop instruction ( so the next symbol can start at the address 3f410 ).
```
Before:                         After:

3f3fe: mov %rdi, %rbx           3f3fe: mov %rdi, %rbx
3f401: syscall                /-3f401: jmp 3f430
3f403: jmp 3f415              | 3f403: jmp 3f415 ----------\
3f407: retq                   | 3f407: retq                |
                              \                            |
3f408: nopl 0x0(%rax,%rax,1)  /-3f408: jmp e1000           |
                              | ...                        |
                              | ...                        |
                              \_...                        |
                                e1000: nop                 |
                                ...                        |
                                e1100: call implementation /
                                ...                       /
                                e1200: jmp 3f403 ________/

```

# Limitations: #
* Only Linux is supported
* Only x86\_64 is supported
* Only tested with glibc, altought perhaps it works
with some other libc implementations as well
* The following syscalls can not be hooked:
  * vfork
  * clone
  * execve
  * rt_sigreturn
