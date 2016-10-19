
---
layout: manual
Content-Style: 'text/css'
title: mkfs.pmemfile(1)
header: NVM Library
date: pmemfile
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

[comment]: <> (mkfs.pmemfile.1.md -- man page for pmempool-check)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[OPTIONS](#options)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**mkfs.pmemfile** -- Create a pmemfile filesystem


# SYNOPSIS #

```
$ mkfs.pmemfile [<options>] path fs-size
```

# DESCRIPTION #

**mkfs.pmemfile** is used to create a file, which can be used as a filesystem
with **pmemfile(3)**. The file system size is specified by fs-size. If
fs-size does not have a suffix, it is interpreted as a number of bytes. If
the fs-size is suffixed by 'k', 'm', 'g', 't' (either upper-case or
lower-case), then it is interpreted in kilobytes, megabytes, gigabytes, etc.
Note, that pmemfile uses at least 8 megabytes for metadata, so any filesystem
must be at least 8 megabytes in size.

# OPTIONS #

`-v, --verbose`

Print version information and exit.

`-h, --help`

Display help message and exit.

# SEE ALSO #

**libpmemfile**(3), **libpmemobj**(3)
and **<http://pmem.io>**
