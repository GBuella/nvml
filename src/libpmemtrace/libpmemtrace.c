
#define _GNU_SOURCE
#include <stddef.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>

#include "libpmemobj.h"
#include "pmemobj_formats.h"

#include "uint64_avl.h"

#define FATAL(x) do { err(1, x); } while (false);
#include "sys_util.h"

#include <valgrind/drd.h>

#include "libpmemtrace.h"

#define PMEM_TX_BEGIN_MAX_ARGS 32

static FILE *output;
static bool shall_close_output;
static struct uint64_avl new_oid_tree;
static struct uint64_avl used_oid_tree;
static pthread_mutex_t oid_store_mutex = PTHREAD_MUTEX_INITIALIZER;

void
pmemtrace_setup_forward(void **forward, const char func[])
{
	static pthread_mutex_t dlsym_mutex = PTHREAD_MUTEX_INITIALIZER;

	/*
	 * Valgrind ignore:
	 * Reading the pointer is supossed to be atomic,
	 * and only one value can ever written to it.
	 * Thus, the worst that can happen, is dlsym being called
	 * more than once with the same symbol name as argument,
	 * and the same address being stored multiple times
	 * into *forward.
	 */
	util_mutex_lock(&dlsym_mutex);
	if (*forward == NULL) {
		*forward = dlsym(RTLD_NEXT, func);
		if (*forward == NULL)
			err(1, "forwarding call to %s", func);
	}
	util_mutex_unlock(&dlsym_mutex);
}

static __attribute__((constructor)) void
libpmemtrace_init(void)
{
	output = stderr;

	const char *path = getenv("PMEMTRACE_PATH");
	if (path != NULL) {
		FILE *file = fopen(path, "a");
		if (file != NULL) {
			output = file;
			shall_close_output = true;
		}
		else {
			perror("opening file specified by PMEMTRACE_PATH");
		}
	}

	uint64_avl_init(&new_oid_tree);
	uint64_avl_init(&used_oid_tree);
}

static __attribute__((destructor)) void
cleanup(void)
{
	/* Valgrind complains about some memory
	 * leaking in libc unless the file
	 * is closed explicitly
	 */
	if (shall_close_output)
		fclose(output);
}

static void
pmemtrace_puts(const char str[])
{
#	ifdef SUPPRESS_FPUTS_DRD_ERROR
	ANNOTATE_IGNORE_READS_AND_WRITES_BEGIN();
#	endif

	(void) fputs(str, output);

#	ifdef SUPPRESS_FPUTS_DRD_ERROR
	ANNOTATE_IGNORE_READS_AND_WRITES_END();
#	endif
}

void
pmemtrace_log(const char format[], ...)
{
	char str[0x800];
	int len;

	va_list ap;
	va_start(ap, format);
	len = vsnprintf(str, sizeof(str) - 1, format, ap);
	va_end(ap);

	str[len] = '\n';
	str[len+1] = '\0';
	pmemtrace_puts(str);
}

void
pmemobj_set_funcs(
		void *(*malloc_func)(size_t size),
		void (*free_func)(void *ptr),
		void *(*realloc_func)(void *ptr, size_t size),
		char *(*strdup_func)(const char *s))
{
	static void (*forward)();
	static const char unkown[] = "unkown";
	const char *malloc_name = unkown;
	const char *free_name = unkown;
	const char *realloc_name = unkown;
	const char *strdup_name = unkown;
	Dl_info info;

	pmemtrace_setup_forward((void**)&forward, __func__);

	if (dladdr(malloc_func, &info) && info.dli_sname != NULL)
		malloc_name = info.dli_sname;
	if (dladdr(free_func, &info) && info.dli_sname != NULL)
		free_name = info.dli_sname;
	if (dladdr(realloc_func, &info) && info.dli_sname != NULL)
		realloc_name = info.dli_sname;
	if (dladdr(strdup_func, &info) && info.dli_sname != NULL)
		strdup_name = info.dli_sname;

	pmemtrace_log("pmemobj_set_funcs(%p:%s, %p:%s, %p:%s, %p:%s) = (void)",
		malloc_func, malloc_name,
		free_func, free_name,
		realloc_func, realloc_name,
		strdup_func, strdup_name);

	forward(malloc_func, free_func, realloc_func, strdup_func);
}

int
pmemobj_tx_begin(PMEMobjpool *pop, jmp_buf env, ...)
{
	static int (*forward)();
	void *locks[PMEM_TX_BEGIN_MAX_ARGS];
	enum pobj_tx_lock lock_types[PMEM_TX_BEGIN_MAX_ARGS];
	char log_buffer[PMEM_TX_BEGIN_MAX_ARGS * 40];
	char *cursor = log_buffer;
	size_t lock_count = 0;
	int result;
	va_list argp;

	pmemtrace_setup_forward((void**)&forward, __func__);

	cursor += sprintf(cursor, "pmemobj_tx_begin(%p, (jmp_buf)",
			(void*) pop);

	va_start(argp, env);
	while (lock_count < PMEM_TX_BEGIN_MAX_ARGS) {
		lock_types[lock_count] = va_arg(argp, enum pobj_tx_lock);
		if (lock_types[lock_count] == TX_LOCK_NONE)
			break;

		locks[lock_count] = va_arg(argp, void*);

		cursor += snprintf(cursor,
			sizeof(log_buffer) - (size_t)(cursor - log_buffer),
			", %d, %p",
			lock_types[lock_count], (void*)locks[lock_count]);

		++lock_count;
	}
	va_end(argp);

	if (lock_count == PMEM_TX_BEGIN_MAX_ARGS)
		FATAL("pmemobj_tx_begin hook function failure");

	result = forward(pop, env,
		//{{{
		lock_types[0],  locks[0],  lock_types[1],  locks[1],
		lock_types[1],  locks[1],  lock_types[2],  locks[2],
		lock_types[2],  locks[2],  lock_types[3],  locks[3],
		lock_types[3],  locks[3],  lock_types[4],  locks[4],
		lock_types[4],  locks[4],  lock_types[5],  locks[5],
		lock_types[5],  locks[5],  lock_types[6],  locks[6],
		lock_types[6],  locks[6],  lock_types[7],  locks[7],
		lock_types[7],  locks[7],  lock_types[8],  locks[8],
		lock_types[8],  locks[8],  lock_types[9],  locks[9],
		lock_types[9],  locks[9],  lock_types[10], locks[10],
		lock_types[10], locks[10], lock_types[11], locks[11],
		lock_types[11], locks[11], lock_types[12], locks[12],
		lock_types[12], locks[12], lock_types[13], locks[13],
		lock_types[13], locks[13], lock_types[14], locks[14],
		lock_types[14], locks[14], lock_types[15], locks[15],
		lock_types[15], locks[15], lock_types[16], locks[16],
		lock_types[16], locks[16], lock_types[17], locks[17],
		lock_types[17], locks[17], lock_types[18], locks[18],
		lock_types[18], locks[18], lock_types[19], locks[19],
		lock_types[19], locks[19], lock_types[20], locks[20],
		lock_types[20], locks[20], lock_types[21], locks[21],
		lock_types[21], locks[21], lock_types[22], locks[22],
		lock_types[22], locks[22], lock_types[23], locks[23],
		lock_types[23], locks[23], lock_types[24], locks[24],
		lock_types[24], locks[24], lock_types[25], locks[25],
		lock_types[25], locks[25], lock_types[26], locks[26],
		lock_types[26], locks[26], lock_types[27], locks[27],
		lock_types[27], locks[27], lock_types[28], locks[28],
		lock_types[28], locks[28], lock_types[29], locks[29],
		lock_types[29], locks[29], lock_types[30], locks[30],
		lock_types[30], locks[30], lock_types[31], locks[31]);
		//}}}

	cursor += snprintf(cursor,
		sizeof(log_buffer) - (size_t)(cursor - log_buffer),
		") = %d",
		result);

	pmemtrace_puts(log_buffer);

	return result;
}

void
pmemtrace_oid_create(PMEMoid oid)
{
	if (OID_IS_NULL(oid))
		return;

	util_mutex_lock(&oid_store_mutex);
	uint64_avl_insert(&new_oid_tree, oid.pool_uuid_lo + oid.off);
	uint64_avl_remove(&used_oid_tree, oid.pool_uuid_lo + oid.off);
	util_mutex_unlock(&oid_store_mutex);
}

void
pmemtrace_oid_use(PMEMoid oid)
{
	if (OID_IS_NULL(oid))
		return;

	util_mutex_lock(&oid_store_mutex);
	uint64_avl_insert(&used_oid_tree, oid.pool_uuid_lo + oid.off);
	uint64_avl_remove(&new_oid_tree, oid.pool_uuid_lo + oid.off);
	util_mutex_unlock(&oid_store_mutex);
}

void
pmemtrace_oid_release(PMEMoid oid)
{
	if (OID_IS_NULL(oid))
		return;

	util_mutex_lock(&oid_store_mutex);
	if (uint64_avl_containts(&new_oid_tree, oid.pool_uuid_lo + oid.off)) {
		pmemtrace_log("object released without use: {"
			".pool_uuid_lo=0x%" PRIx64 ", .off=%" PRIu64 "}",
			oid.pool_uuid_lo, oid.off);
		uint64_avl_remove(&new_oid_tree, oid.pool_uuid_lo);
	}
	uint64_avl_remove(&used_oid_tree, oid.pool_uuid_lo + oid.off);
	util_mutex_unlock(&oid_store_mutex);
}

void*
hook_pmemobj_direct(PMEMoid oid, void* (*forward)(PMEMoid))
{
	pmemtrace_oid_use(oid);
	return forward(oid);
}

/* In the case of pmemobj_free,
 * the argument must be logged before the call,
 * since the integer ids are cleared by the call.
 * Thus it is another exception, not handled by
 * the generate_forwarders.sed script
 */
void
pmemobj_free(PMEMoid *oidp)
{
	pmemtrace_log(format_pmemobj_free, oidp,
		oidp ? oidp->pool_uuid_lo : 0,
		oidp ? oidp->off : 0);
	static void (*forward)(PMEMoid*);

	pmemtrace_setup_forward((void**)&forward, __func__);
	if (oidp != NULL)
		pmemtrace_oid_release(*oidp);

	forward(oidp);
}

