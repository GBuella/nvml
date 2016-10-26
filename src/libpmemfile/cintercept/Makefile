# Copyright 2016, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# This library contains code from the Netwide Assembler project
# See http://nasm.us
#
# Copyright 1996-2016 the NASM Authors - All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following
# conditions are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the following
#   disclaimer in the documentation and/or other materials provided
#   with the distribution.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
#   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
#   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
#   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# src/libpmemfile/cinterceptlib/Makefile
# Makefile for libpmemfile's intercepting code
#

LIBRARY_NAME = cintercept
LIBRARY_SO_VERSION = 1
LIBRARY_VERSION = 0.0

NASM_GENERATED_SOURCES = \
	nasm/x86/regs.c \
	nasm/x86/regflags.c \
	nasm/x86/regdis.c \
	nasm/x86/regvals.c \
	nasm/x86/iflag.c \
	nasm/x86/insnsb.c \
	nasm/x86/insnsa.c \
	nasm/x86/insnsd.c \
	nasm/x86/insnsn.c


SOURCE = \
	intercept.c \
	patcher.c \
	intercept_util.c \
	intercept_desc.c \
	disasm_wrapper.c \
	disasm.c \
	../../libpmem/cpu.c \
	$(NASM_GENERATED_SOURCES) \
	nasm/x86/disp8.c

ASMSOURCE = \
	intercept_template.s \
	syscall_no_intercept.s

include ../../Makefile.inc

CFLAGS += -Inasm/x86 -Inasm/include -Inasm/asm
LIBS += -ldl

$(objdir)/disasm.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/regs.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/regflags.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/regdis.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/regvals.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/iflag.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/insnsb.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/insnsa.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/insnsd.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/insnsn.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion
$(objdir)/nasm/x86/disp8.o: CFLAGS+=-Wno-sign-conversion -Wno-conversion

$(objdir)/disasm.o: \
	nasm/x86/iflaggen.h \
	nasm/x86/insnsi.h \
	nasm/x86/regdis.h \
	nasm/x86/regs.h


$(objdir)/disasm_wrapper.o: \
	nasm/x86/iflaggen.h \
	nasm/x86/insnsi.h \
	nasm/x86/regdis.h \
	nasm/x86/regs.h

# TODO: get rid of this hack!
../../debug/cintercept/nasm/x86/.placeholder:
	mkdir -p ../../debug/cintercept/nasm/x86
	mkdir -p ../../nondebug/cintercept/nasm/x86
	mkdir -p ../../debug/cintercept/.deps/nasm/x86
	mkdir -p ../../nondebug/cintercept/.deps/nasm/x86
	> ../../debug/cintercept/nasm/x86/.placeholder

clean_nasm_gen_srcs:
	$(RM) $(NASM_GENERATED_SOURCES) \
	nasm/x86/insnsi.h \
	nasm/x86/iflaggen.h \
	nasm/x86/regdis.h \
	nasm/x86/regs.h

$(objdir)/disasm.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/regs.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/regflags.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/regdis.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/regvals.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/iflag.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/insnsb.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/insnsa.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/insnsd.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/insnsn.o: $(NASM_GENERATED_SOURCES)
$(objdir)/nasm/x86/disp8.o: $(NASM_GENERATED_SOURCES)


clean: clean_nasm_gen_srcs

#
# The following rules have been lifted over from nasm's Makefile
#

# These source files are automagically generated from a single
# instruction-table file by a Perl script. They're distributed,
# though, so it isn't necessary to have Perl just to recompile NASM
# from the distribution.
INSDEP = nasm/x86/insns.dat nasm/x86/insns.pl \
	../../debug/cintercept/nasm/x86/.placeholder

nasm/x86/iflag.c: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -fc nasm/x86/insns.dat $@
nasm/x86/iflaggen.h: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -fh nasm/x86/insns.dat $@
nasm/x86/insnsb.c: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -b nasm/x86/insns.dat $@
nasm/x86/insnsa.c: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -a nasm/x86/insns.dat $@
nasm/x86/insnsd.c: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -d nasm/x86/insns.dat $@
nasm/x86/insnsi.h: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -i nasm/x86/insns.dat $@
nasm/x86/insnsn.c: $(INSDEP)
	PERL5LIB=nasm perl nasm/x86/insns.pl -n nasm/x86/insns.dat $@

# These source files are generated from regs.dat by yet another
# perl script.
REGSDEP = nasm/x86/regs.dat nasm/x86/regs.pl \
	../../debug/cintercept/nasm/x86/.placeholder

nasm/x86/regs.c: $(REGSDEP)
	PERLLIB=nasm perl nasm/x86/regs.pl c nasm/x86/regs.dat > $@
nasm/x86/regflags.c: $(REGSDEP)
	PERL5LIB=nasm perl nasm/x86/regs.pl fc nasm/x86/regs.dat > $@
nasm/x86/regdis.c: $(REGSDEP)
	PERL5LIB=nasm perl nasm/x86/regs.pl dc nasm/x86/regs.dat > $@
nasm/x86/regdis.h: $(REGSDEP)
	PERL5LIB=nasm perl nasm/x86/regs.pl dh nasm/x86/regs.dat > $@
nasm/x86/regvals.c: $(REGSDEP)
	PERL5LIB=nasm perl nasm/x86/regs.pl vc nasm/x86/regs.dat > $@
nasm/x86/regs.h: $(REGSDEP)
	PERL5LIB=nasm perl nasm/x86/regs.pl h nasm/x86/regs.dat > $@
