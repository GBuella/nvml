
##### Copyright 2014-2016, Intel Corporation #####

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

##### pmemfile - syscall routing #####

The library makes observable to a process one or more filesystems,
mounted at existing directories in the kernel provided virtual file system.
The application can access the contents of a pmemfile pool by accessing
paths under these directories. Such use is meant to be transparent, in
a sense that a process should not behave differently wether some files
are provided by pmemfile, or the kernel at the same path.

In the case of each file operation related intercepted syscall,
the preloadable library must decide if the syscall should be handled
by pmemfile, or by the kernel. Most of such syscalls belong to one of two
groups:

1) those with one or more arguments refering to path(s)
2) those with one or more arguments refering to file descriptor number(s)

##### Path resolution #####

Whenever a syscall with a path argument ( e.g.: open, chdir, link, etc... )
is intercepted, the path is eximend to see if it refers to something under
a pmemfile mount point. The method resembles the one used in realpath(3),
with the exception of paths that start with a string which compares
equal to a mount point -- in such cases the mount point is stripped
from the begining of the path, and the appropriate pmemfile pool is
queried for symlinks, instead of the kernel.
One example:

Let "/mnt/pmem_data" be a mount point for a pmemfile pool.
Let "/home/joe" be the current working directory.
Let "some_link/../mnt/pmem_data/dir/file" be the path to be resolved.

The following steps are taken to resolve the path:

1) The path is prepended with the current working directory:
"/home/joe/some_link/../mnt/pmem_data/dir/file"

2) The first few components are resolved by quering the kernel:
  * Next component to be resolved: "/home"
  stat("/home")
  * Next component to be resolved: "joe"
  stat("/home/joe")
  * Next component to be resolved: "some_link"
  stat("/home/joe/some_link")
  readlink("/home/joe/some_link") -> /mnt/volume_1
   the full path to be resolved changes to "/mnt/volume_1/../pmem_data/dir/file"
  * Next component to be resolved: "/mnt"
  stat("/mnt")
  * Next component to be resolved: "volume_1"
  stat("/mnt/volume_1")
  * Next component to be resolved: ".."
   ".." is resolved without a syscall
   At this point the full path to be resolved is "/mnt/pmem_data/dir/file"
  * Next component to be resolved: "pmem_data"
   Which is recognized to be the root of pmemfile provided filesystem


3) The pmemfile resident paths are resolved by queried pmemfile corelib:
Since pmemfile corelib is not expected to know about / depend on
paths not provided by its filesystem, the virtual mount point is stripped
from each path given to corelib.
  * Next component to be resolved: "dir"
  pmemfile_stat("/dir") -- instead of pmemfile_stat("/mnt/pmem_data/dir")
  * Next component to be resolved: "file"
  pmemfile_stat("/dir/file")

5) The result of the path resolution is decision about forwarding the
syscall to pmemfile. In short, the path:
"/home/joe/some_link/../mnt/pmem_data/dir/file"
is forwarded to pmemfile corelib as:
"/dir/file"

##### File descriptor resolution #####

A file descriptor can refer to pmemfile resident file, or a file
opened by a syscall forwarded to the kernel. Such distinction should
not be noticable by the application in normal circumstances ( e.g.:
the application not checking /proc/fd/number ). The syscall routing
allocates a number of file descriptors for use only with pmemfile
resident files, by the open syscall. In the current version, the
path "/dev/null" is opened acquire file descriptors from the kernel,
and thereafter, the library does never forward these file descriptors
to the kernel. Without this, it would be rather difficult to
make sure that kernel resident files and pmemfile resident files
use a distinct set of file descriptors. Every open syscall handled
by
* either being forwarded to the kernel, and returning the fd
returned by the kernel
* or being forwarded to pmemfile, and returning an fd allocated by
opening "/dev/null"

In the second case, the syscall routing library must keep track of
not only having an specially allocated file descriptor, but also
the association between this file descriptor and file being open
in a pmemfile pool.

##### Internal mutexes used #####

TBD

( What can be handled in paralell, when do threads need to wait
for another thread's file operation? )

##### thread safety, signal safety, cancel safety #####

TBD

( We handle any calls from any number of threads
-- well, at least we definitely should )
( No problem with signals -- I hope )
( We talked about thread cancellation points -- ignored for now )

##### Stack usage #####

TBD

( The maximum stack usage of the application grows by a few kilobytes,
this is not a strictly transparent change.
due to the extra layers: intercepting library, syscall routing, pmemfile
corelib, pmemobj )
( But usually this should not matter, in extreme cases just set a
higher stack limit by `ulimit -s`)

##### Call stack #####

TBD

( The application's callstack is not available when something
in pmemfile crashes - the callstack only shows callstack entries below libc )

( Question: can this be helped? )


!!! pmemfile can crash if the application supplies an invalid pointer
as a path argument, in this case, the user only sees the pmemfile crashed,
not given a chance to see the problem originating in the application.

##### Access from multiple processes, fork, etc... #####

TBD

( Not really handled at all right now )

##### testing #####

TBD

* mkfs.pmemfile
* pmemfile-cat
* pmemfile-ls
* pmemfile-stat
