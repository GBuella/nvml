
#ifndef LIBPMEMTRACE_H
#define LIBPMEMTRACE_H

#include "libpmemobj.h"

void pmemtrace_setup_forward(void **forward, const char *func);

void pmemtrace_log(const char format[], ...)
	__attribute__((format(printf, 1, 2)));


void pmemtrace_oid_create(PMEMoid oid);
void pmemtrace_oid_use(PMEMoid oid);
void pmemtrace_oid_release(PMEMoid oid);

void* hook_pmemobj_direct(PMEMoid oid, void* (*forward)(PMEMoid));
#define HOOK_pmemobj_direct hook_pmemobj_direct

void hook_pmemobj_free(PMEMoid *oidp, void (*forward)(PMEMoid*));
#define HOOK_pmemobj_free hook_pmemobj_free

#endif
