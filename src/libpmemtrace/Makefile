
# vim: set noet:

LIBRARY_NAME = pmemtrace
LIBRARY_SO_VERSION = 0
LIBRARY_VERSION = 0.1

SOURCE = pmem_forwarders.c pmemobj_forwarders.c libpmemtrace.c uint64_avl.c

all: pmem_formats.h pmemobj_formats.h \
	pmem_forwarders.c pmemobj_forwarders.c

include ../Makefile.inc

pmem_formats.h: ../include/libpmem.h
	sed -f filter_functions.sed < ../include/libpmem.h | \
	sed -f format_strings.sed > $@

pmemobj_formats.h: ../include/libpmemobj.h
	sed -f filter_functions.sed < ../include/libpmemobj.h | \
	sed -f format_strings.sed > $@

pmem_forwarders.c: ../include/libpmem.h
	sed -f filter_functions.sed < ../include/libpmem.h | \
	sed -f generate_forwarders.sed > $@

pmemobj_forwarders.c: ../include/libpmemobj.h
	sed -f filter_functions.sed < ../include/libpmemobj.h | \
	sed -f generate_forwarders.sed > $@


pmem_formats.h: filter_functions.sed format_strings.sed
pmemobj_formats.h: filter_functions.sed format_strings.sed
pmem_forwarders.c: filter_functions.sed generate_forwarders.sed
pmemobj_forwarders.c: filter_functions.sed generate_forwarders.sed

LIBS += -ldl
