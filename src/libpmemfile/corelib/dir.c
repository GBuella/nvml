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

/*
 * dir.c -- directory operations
 */

#include <errno.h>
#include <stdio.h>

#include "internal.h"
#include "util.h"

/*
 * file_set_path_debug_locked -- (internal) sets full path in runtime structures
 * of child_inode based on parent inode and name.
 *
 * Works only in DEBUG mode.
 * Assumes child inode is already locked.
 */
static void
file_set_path_debug_locked(PMEMfilepool *pfp,
		TOID(struct pmemfile_inode) parent_inode,
		TOID(struct pmemfile_inode) child_inode,
		const char *name)
{
#ifdef	DEBUG
	struct pmemfile_inode_rt *child_rt =
			pmemfile_inode_get(pfp, child_inode);

	if (child_rt->path)
		return;

	if (TOID_IS_NULL(parent_inode)) {
		child_rt->path = Strdup(name);
		return;
	}

	struct pmemfile_inode_rt *parent_rt =
			pmemfile_inode_get(pfp, parent_inode);

	if (strcmp(parent_rt->path, "/") == 0) {
		child_rt->path = Malloc(strlen(name) + 2);
		sprintf(child_rt->path, "/%s", name);
		return;
	}

	char *p = Malloc(strlen(parent_rt->path) + 1 + strlen(name) + 1);
	sprintf(p, "%s/%s", parent_rt->path, name);
	child_rt->path = p;
#endif
}

/*
 * pmemfile_set_path_debug -- (internal) sets full path in runtime structures
 * of child_inode based on parent inode and name.
 */
void
pmemfile_set_path_debug(PMEMfilepool *pfp,
		TOID(struct pmemfile_inode) parent_inode,
		TOID(struct pmemfile_inode) child_inode,
		const char *name)
{
	struct pmemfile_inode_rt *child_rt =
			pmemfile_inode_get(pfp, child_inode);
	pmemfile_inode_wlock(child_rt);

	file_set_path_debug_locked(pfp, parent_inode, child_inode, name);

	pmemfile_inode_unlock(child_rt);
}

/*
 * pmemfile_add_dentry -- adds child inode to parent directory
 *
 * Must be called in transaction. Caller must have exclusive access to parent
 * inode, by locking parent in WRITE mode.
 */
void
pmemfile_add_dentry(PMEMfilepool *pfp,
		TOID(struct pmemfile_inode) parent_inode,
		const char *name,
		TOID(struct pmemfile_inode) child_inode,
		const struct timespec *tm)
{
	LOG(LDBG, "parent 0x%lx ppath %s name %s child_inode 0x%lx",
			parent_inode.oid.off, pmfi_path(parent_inode), name,
			child_inode.oid.off);

	ASSERTeq(pmemobj_tx_stage(), TX_STAGE_WORK);

	if (strlen(name) > PMEMFILE_MAX_FILE_NAME) {
		LOG(LUSR, "file name too long");
		pmemobj_tx_abort(EINVAL);
	}

	struct pmemfile_inode *parent = D_RW(parent_inode);

	struct pmemfile_dir *dir = D_RW(parent->file_data.dir);
	ASSERTne(dir, NULL);

	struct pmemfile_dentry *dentry = NULL;
	bool found = false;

	do {
		for (unsigned i = 0; i < NUMDENTRIES; ++i) {
			if (strcmp(dir->dentries[i].name, name) == 0)
				pmemobj_tx_abort(EEXIST);

			if (!found && dir->dentries[i].name[0] == 0) {
				dentry = &dir->dentries[i];
				TX_ADD_DIRECT(&dir->used);
				dir->used++;
				found = true;
			}
		}

		if (!found && TOID_IS_NULL(dir->next))
			TX_SET_DIRECT(dir, next, TX_ZNEW(struct pmemfile_dir));

		dir = D_RW(dir->next);
	} while (dir);

	TX_ADD_DIRECT(dentry);

	dentry->inode = child_inode;

	strncpy(dentry->name, name, PMEMFILE_MAX_FILE_NAME);
	dentry->name[PMEMFILE_MAX_FILE_NAME] = '\0';

	TX_ADD_FIELD(child_inode, nlink);
	D_RW(child_inode)->nlink++;

	/*
	 * From "stat" man page:
	 * "The field st_ctime is changed by writing or by setting inode
	 * information (i.e., owner, group, link count, mode, etc.)."
	 */
	TX_SET(child_inode, ctime, *tm);

	/*
	 * From "stat" man page:
	 * "st_mtime of a directory is changed by the creation
	 * or deletion of files in that directory."
	 */
	TX_SET(parent_inode, mtime, *tm);

	pmemfile_set_path_debug(pfp, parent_inode, child_inode, name);
}

/*
 * pmemfile_new_dir -- creates new directory relative to parent
 *
 * Note: caller must hold WRITE lock on parent.
 */
TOID(struct pmemfile_inode)
pmemfile_new_dir(PMEMfilepool *pfp, TOID(struct pmemfile_inode) parent,
		const char *name)
{
	LOG(LDBG, "parent 0x%lx ppath %s new_name %s", parent.oid.off,
			pmfi_path(parent), name);

	ASSERTeq(pmemobj_tx_stage(), TX_STAGE_WORK);

	struct timespec t;
	TOID(struct pmemfile_inode) child =
			pmemfile_inode_alloc(pfp, S_IFDIR | 0777, &t);
	pmemfile_set_path_debug(pfp, parent, child, name);

	D_RW(child)->file_data.dir = TX_ZNEW(struct pmemfile_dir);

	/* add . and .. to new directory */
	pmemfile_add_dentry(pfp, child, ".", child, &t);

	if (TOID_IS_NULL(parent)) /* special case - root directory */
		pmemfile_add_dentry(pfp, child, "..", child, &t);
	else
		pmemfile_add_dentry(pfp, child, "..", parent, &t);

	return child;
}

/*
 * file_lookup_dentry_locked -- looks up file name in passed directory
 *
 * Caller must hold lock on parent.
 */
static struct pmemfile_dentry *
file_lookup_dentry_locked(PMEMfilepool *pfp, TOID(struct pmemfile_inode) parent,
		const char *name, struct pmemfile_dir **outdir)
{
	LOG(LDBG, "parent 0x%lx ppath %s name %s", parent.oid.off,
			pmfi_path(parent), name);

	struct pmemfile_inode *par = D_RW(parent);
	if (!_pmemfile_is_dir(par)) {
		errno = ENOTDIR;
		return NULL;
	}

	TOID(struct pmemfile_dir) tdir = par->file_data.dir;

	while (!TOID_IS_NULL(tdir)) {
		struct pmemfile_dir *dir = D_RW(tdir);

		for (unsigned i = 0; i < NUMDENTRIES; ++i) {
			struct pmemfile_dentry *d = &dir->dentries[i];

			if (strcmp(d->name, name) == 0) {
				if (outdir)
					*outdir = dir;
				return d;
			}
		}

		tdir = D_RW(tdir)->next;
	}

	errno = ENOENT;
	return NULL;
}

/*
 * pmemfile_lookup_dentry -- looks up file name in passed directory
 *
 * Takes reference on found inode. Caller must hold reference to parent inode.
 * Does not need transaction.
 */
TOID(struct pmemfile_inode)
pmemfile_lookup_dentry(PMEMfilepool *pfp, TOID(struct pmemfile_inode) parent,
		const char *name)
{
	LOG(LDBG, "parent 0x%lx ppath %s name %s", parent.oid.off,
			pmfi_path(parent), name);

	struct pmemfile_inode_rt *parent_rt = pmemfile_inode_get(pfp, parent);
	TOID(struct pmemfile_inode) inode = TOID_NULL(struct pmemfile_inode);

	pmemfile_inode_rlock(parent_rt);

	struct pmemfile_dentry *dentry =
			file_lookup_dentry_locked(pfp, parent, name, NULL);
	if (dentry) {
		pmemfile_inode_ref_set_path(pfp, dentry->inode, parent, name);
		inode = dentry->inode;
	}

	pmemfile_inode_unlock(parent_rt);

	return inode;
}

/*
 * pmemfile_unlink_dentry -- removes dentry from directory
 *
 * Must be called in transaction. Caller must have exclusive access to parent
 * inode, eg by locking parent in WRITE mode.
 */
void
pmemfile_unlink_dentry(PMEMfilepool *pfp, TOID(struct pmemfile_inode) parent,
		const char *name, volatile struct pmemfile_inode_rt **rt)
{
	LOG(LDBG, "parent 0x%lx ppath %s name %s", parent.oid.off,
			pmfi_path(parent), name);

	struct pmemfile_dir *dir;
	struct pmemfile_dentry *dentry =
			file_lookup_dentry_locked(pfp, parent, name, &dir);

	if (!dentry)
		pmemobj_tx_abort(errno);

	TX_ADD_DIRECT(&dir->used);
	dir->used--;

	TOID(struct pmemfile_inode) tinode = dentry->inode;
	struct pmemfile_inode *inode = D_RW(tinode);

	if (_pmemfile_is_dir(inode))
		pmemobj_tx_abort(EISDIR);

	*rt = pmemfile_inode_ref(pfp, tinode);
	pmemfile_inode_tx_wlock((struct pmemfile_inode_rt *)*rt);

	ASSERT(inode->nlink > 0);

	TX_ADD_FIELD(tinode, nlink);
	TX_ADD_DIRECT(dentry);

	--inode->nlink;

	dentry->name[0] = '\0';
	dentry->inode = TOID_NULL(struct pmemfile_inode);

	pmemfile_inode_unref_locked(pfp, tinode);
	*rt = NULL;
}

/*
 * _pmemfile_list -- dumps directory listing to log file
 *
 * XXX: remove once directory traversal API is implemented
 */
void
_pmemfile_list(PMEMfilepool *pfp, TOID(struct pmemfile_inode) parent)
{
	LOG(LINF, "parent 0x%lx ppath %s", parent.oid.off,
			pmfi_path(parent));

	struct pmemfile_inode *par = D_RW(parent);

	TOID(struct pmemfile_dir) tdir = par->file_data.dir;

	LOG(LINF, "- ref    inode nlink   size   flags name");

	while (!TOID_IS_NULL(tdir)) {
		struct pmemfile_dir *dir = D_RW(tdir);

		for (unsigned i = 0; i < NUMDENTRIES; ++i) {
			const struct pmemfile_dentry *d = &dir->dentries[i];
			if (d->name[0] == 0)
				continue;

			const struct pmemfile_inode *inode = D_RO(d->inode);
			const struct pmemfile_inode_rt *rt;

			if (TOID_EQUALS(parent, d->inode))
				rt = pmemfile_inode_get(pfp, d->inode);
			else
				rt = pmemfile_inode_ref_set_path(pfp, d->inode,
						parent, d->name);

			LOG(LINF, "* %3d 0x%6lx %5lu %6lu 0%06lo %s",
				rt->common.ref, d->inode.oid.off, inode->nlink,
				inode->size, inode->flags, d->name);

			if (!TOID_EQUALS(parent, d->inode))
				pmemfile_inode_unref_tx(pfp, d->inode);
		}

		tdir = D_RW(tdir)->next;
	}
}
