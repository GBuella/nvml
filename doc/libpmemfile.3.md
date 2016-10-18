---
layout: manual
Content-Style: 'text/css'
title: libpmemfile(3)
header: NVM Library
date: pmemfile API version 0.0.1
...

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

[comment]: <> (libpmemfile.3 -- man page for preloadable pmemfile library)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[UNDER THE HOOD](#under-the-hood)<br />
[LIMITATIONS](#limitations)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**libpmemfile** -- User space file system using persistent memory

# SYNOPSIS #

```sh
PMEMFILE_POOLS=mount_point:poolfile LD_PRELOAD=libpmemfile.so
```

##### Description: #####

The pmemfile library provides an easy way for applications to use
persistent memory, without any modification to the application
source or binary. This achieved by intercepting all file system
related syscalls issued by the application.

The user must provide an existing pmemfile pool ( created
using mkfs.pmemfile(3) ), and a virtual mount point where
the library should make a file system visible to the application.
In the current version, this can be set up with the help of
environment variables, e.g.: to make the virtual filesystem
residing in the pool /home/joe/pfiles be visible to an
application under /mnt/pfiles, one must run the application
in this manner:
```sh
PMEMFILE_POOLS=/mnt/pfiles:/home/joe/pfiles LD_PRELOAD=libpmemfile.so ./app
```

##### See also: #####

**mkfs.pmemfile(3)**, **libcintercept(3)**, **libpmemfile-core(3)**
