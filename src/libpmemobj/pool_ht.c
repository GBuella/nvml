/*
 * Copyright 2017, Intel Corporation
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

/* XXX Find appropriate VC++ equivalents of sched_yield(), atomic_*() */

#include <stdbool.h>
#include <sched.h>

#include "pool_ht.h"
#include "out.h"
#include "sys_util.h"

#ifndef PMEMOBJ_POOL_HT_SIZE_LOG2
#define PMEMOBJ_POOL_HT_SIZE_LOG2 12
#endif

#if PMEMOBJ_POOL_HT_SIZE_LOG2 < 1
#error Invalid PMEMOBJ_POOL_HT_SIZE_LOG2
#endif

#if PMEMOBJ_POOL_HT_SIZE_LOG2 > 30
#error Invalid PMEMOBJ_POOL_HT_SIZE_LOG2
#endif

#define SLOT_COUNT (((size_t)1) << PMEMOBJ_POOL_HT_SIZE_LOG2)

struct slot {
	uint64_t key;
	void *value;
};

static struct slot table[SLOT_COUNT];

static uint64_t
load_key(uint64_t index)
{
	return __atomic_load_n(&table[index].key, __ATOMIC_ACQUIRE);
}

static void *
load_value(uint64_t index)
{
	return __atomic_load_n(&table[index].value, __ATOMIC_ACQUIRE);
}

static void
slot_store(uint64_t index, uint64_t key, void *value)
{
	__atomic_store_n(&table[index].key, key, __ATOMIC_RELEASE);
	__atomic_store_n(&table[index].value, value, __ATOMIC_RELEASE);
}

static size_t used_slot_count;

static uint64_t status_indicator;
static os_mutex_t modification_guard;
os_once_t mguard_init_once = OS_ONCE_INIT;

static uint64_t
status_load(void)
{
	return __atomic_load_n(&status_indicator, __ATOMIC_ACQUIRE);
}

static void
status_increment(void)
{
	__atomic_add_fetch(&status_indicator, 1, __ATOMIC_RELEASE);
}

static bool
indicates_modification_in_progress(uint64_t status)
{
	return (status & 1) != 0;
}

static uint64_t
hash_key(uint64_t key)
{
	return key; /* XXX */
}

static uint64_t
find_slot_index(uint64_t uuid_lo, uint64_t hash)
{
	uint64_t index = hash % SLOT_COUNT;

	uint64_t key = load_key(index);
	while (key != 0 && key != uuid_lo) {
		index = (index + 1) % SLOT_COUNT;
		key = load_key(index);
	}

	return index;
}

/*
 * pool_ht_get - get the pool pointer
 *
 * Fast path (when there are no modifications):
 *  - read status_indicator
 *  - check bit #0 in status
 *  - find appropirate slot
 *  - read result from appropirate slot
 *  - check status_indicator
 *  - return result
 */
void *
pool_ht_get(uint64_t uuid_lo)
{
	uint64_t hash = hash_key(uuid_lo);

	uint64_t status_seen;
	void *result;

	do {
		status_seen = status_load();

		while (indicates_modification_in_progress(status_seen)) {
			sched_yield();
			status_seen = status_load();
			/* restart when modification in progress */
		}

		result = load_value(find_slot_index(uuid_lo, hash));

		/* restart when modification is detected */
	} while (status_seen != status_load());

	return result;
}

size_t
pool_ht_get_size(void)
{
	return used_slot_count;
}

static void
init_mguard_mutex(void)
{
	util_mutex_init(&modification_guard);
}

int
pool_ht_insert(uint64_t uuid_lo, void *pop)
{
	int r;

	os_once(&mguard_init_once, init_mguard_mutex);
	os_mutex_lock(&modification_guard);

	if (used_slot_count >= SLOT_COUNT - 1)
		return -1;

	uint64_t hash = hash_key(uuid_lo);
	uint64_t index = find_slot_index(uuid_lo, hash);

	if (load_key(index) == 0) {
		status_increment();
		slot_store(index, uuid_lo, pop);
		status_increment();
		r = 0;
		++used_slot_count;
	} else {
		r = -1;
	}

	os_mutex_unlock(&modification_guard);

	return r;
}

int
pool_ht_remove(uint64_t uuid_lo)
{
	int r;

	os_once(&mguard_init_once, init_mguard_mutex);
	os_mutex_lock(&modification_guard);

	uint64_t hash = hash_key(uuid_lo);
	uint64_t index = find_slot_index(uuid_lo, hash);

	if (load_key(index) == uuid_lo) {
		status_increment();
		slot_store(index, 0, NULL);
		status_increment();
		r = 0;
		--used_slot_count;
	} else {
		r = -1;
	}

	os_mutex_unlock(&modification_guard);

	return r;
}
