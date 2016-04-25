
#define	_GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <err.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "uint64_avl.h"

#define	PAGESIZE ((size_t)0x1000)

#pragma GCC diagnostic ignored "-Wconversion"

struct node {
	uint64_t value;
	int64_t left:29;
	int64_t right:29;
	uint8_t height:6;
};

static struct node *
first_node(struct uint64_avl *tree)
{
	return ((struct node *)(tree->data));
}

static struct node *
last_node(struct uint64_avl *tree)
{
	return first_node(tree) + (tree->count - 1);
}

static void *
allocate_new_page(void)
{
	void *address;

	address = mmap(NULL, PAGESIZE,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (address == MAP_FAILED)
		err(1, "Unable to allocate memory for oid storage");

	return address;
}

static void *
grow(void *address, size_t original_size)
{
	if (original_size == 0) {
		return allocate_new_page();
	} else {
		address = mremap(address, original_size,
			original_size + PAGESIZE,
			MREMAP_MAYMOVE);
		if (address == MAP_FAILED)
			err(1, "Unable to allocate memory for oid storage");
		return address;
	}
}

static void *
shrink(void *address, size_t original_size)
{
	if (original_size == 0) {
		return NULL;
	} else if (original_size == PAGESIZE) {
		(void) munmap(address, original_size);
		return NULL;
	} else {
		address = mremap(address, original_size,
			original_size - PAGESIZE,
			MREMAP_MAYMOVE);
		if (address == MAP_FAILED)
			err(1, "Unable to allocate memory for oid storage");
		return address;
	}
}

void
uint64_avl_init(struct uint64_avl *tree)
{
	tree->count = 0;
	tree->memory_usage = 0;
	tree->data = NULL;
}

void
uint64_avl_destroy(struct uint64_avl *tree)
{
	if (tree->memory_usage != 0)
		(void) munmap(tree->data, tree->memory_usage);

	memset(tree, 0, sizeof (*tree));
}

static void
setup_height(struct node *node)
{
	uint8_t max = 0;

	if (node->left != 0)
		max = node[node->left].height;

	if (node->right != 0) {
		if (node[node->right].height > max)
			max = node[node->right].height;
	}

	node->height = max;
	node->height++;
}

static void
rotate_left(struct node *root)
{
	assert(root->right != 0);
	assert(root->right != root->left);

	struct node orig_root = *root;
	struct node *orig_right = root + root->right;

	root->value = orig_right->value;
	orig_right->value = orig_root.value;

	if (orig_right->right != 0)
		root->right = (orig_right + orig_right->right) - root;
	else
		root->right = 0;
	root->left = orig_root.right;

	orig_right->right = orig_right->left;
	if (orig_root.left != 0)
		orig_right->left = (root + orig_root.left) - orig_right;
	else
		orig_right->left = 0;

	setup_height(orig_right);

	assert(orig_right + orig_right->left != root);
	assert(orig_right + orig_right->right != root);
}

static void
rotate_right(struct node *root)
{
	assert(root->left != 0);
	assert(root->right != root->left);

	struct node orig_root = *root;
	struct node *orig_left = root + root->left;

	root->value = orig_left->value;
	orig_left->value = orig_root.value;

	if (orig_left->left != 0)
		root->left = (orig_left + orig_left->left) - root;
	else
		root->left = 0;
	root->right = orig_root.left;

	orig_left->left = orig_left->right;
	if (orig_root.right != 0)
		orig_left->right = (root + orig_root.right) - orig_left;
	else
		orig_left->right = 0;

	setup_height(orig_left);

	assert(orig_left + orig_left->left != root);
	assert(orig_left + orig_left->right != root);
}

static void
rebalance(struct node *root)
{
	int diff = 0;

	if (root->left == 0) {
		if (root->right != 0)
			diff = root[root->right].height;
	} else if (root->right == 0) {
		if (root->left != 0)
			diff = -root[root->left].height;
	} else {
		diff = root[root->right].height - root[root->left].height;
	}

	// cstyle script on "-1": "missing space after - operator"
	//  apparently this script is work in progress
	if (diff < - 1)
		rotate_right(root);
	else if (diff > 1)
		rotate_left(root);
}

/* returns the number of nodes inserted - zero or one */
static int
insert_under(struct node *restrict cursor, const struct node *restrict new)
{
	if (cursor->value == new->value)
		return 0;

	if (cursor->value < new->value) {
		if (cursor->left == 0) {
			cursor->left = new - cursor;
		} else {
			if (insert_under(cursor + cursor->left, new) == 0)
				return 0;
		}
	} else {  //  if cursor->value > new->value
		if (cursor->right == 0) {
			cursor->right = new - cursor;
		} else {
			if (insert_under(cursor + cursor->right, new) == 0)
				return 0;
		}
	}

	rebalance(cursor);
	setup_height(cursor);
	return 1;
}

void
uint64_avl_insert(struct uint64_avl *tree, uint64_t value)
{
	if (tree->count * sizeof (struct node) == tree->memory_usage) {
		tree->data = grow(tree->data, tree->memory_usage);
		tree->memory_usage += PAGESIZE;
	}

	struct node *new = last_node(tree) + 1;

	new->left = new->right = 0;
	new->height = 1;
	new->value = value;

	if (tree->count == 0) {
		tree->count = 1;
	} else {
		if (insert_under(tree->data, new) != 0)
			tree->count++;
	}
}

/*
 * reroute - find the parent pointing to src,
 * and make it point to dst instead
 */
static void
reroute(struct node *restrict cursor,
	const struct node *restrict dst,
	const struct node *restrict src)
{
	if (cursor + cursor->left == src)
		cursor->left += dst - src;
	else if (cursor + cursor->right == src)
		cursor->right += dst - src;
	else if (cursor->value < src->value)
		reroute(cursor + cursor->left, dst, src);
	else
		reroute(cursor + cursor->right, dst, src);
}

/*
 * copy the contents of src to dst,
 * keeping the child pointers correct
 */
static void
overwrite(struct node *restrict dst, const struct node *restrict src)
{
	dst->value = src->value;
	dst->height = src->height;

	if (src->left == 0)
		dst->left = 0;
	else
		dst->left = src->left + (src - dst);

	if (src->right == 0)
		dst->right = 0;
	else
		dst->right = src->right + (src - dst);
}

/*
 * find the node with the lowest value ( leftmost node ),
 * remove it from the subtree, and return the address
 * of the new free slot
 */
static struct node *
remove_from_left(struct node *root)
{
	assert(root->left != 0);

	struct node *child_left = root + root->left;
	struct node *removed;

	if (child_left->left == 0) {
		if (child_left->right == 0)
			root->left = 0;
		else
			root->left += child_left->right;
		removed =  child_left;
	} else {
		removed =  remove_from_left(child_left);
	}
	setup_height(root);
	return removed;
}

/*
 * remove the node from the tree,
 * moving one of its kids into its place
 * if appropriate
 */
static struct node *
remove(struct node *node)
{
	struct node *child_left = node + node->left;
	struct node *child_right = node + node->right;

	if (node->left == 0) {
		if (node->right == 0) {
			return node;
		} else {
			overwrite(node, child_right);
			return child_right;
		}
	} else if (node->right == 0) {
		overwrite(node, child_left);
		return child_left;
	} else {
		if (child_right->left == 0) {
			node->value = child_right->value;
			if (child_right->right != 0)
				node->right += child_right->right;
			else
				node->right = 0;
			setup_height(node);
			return child_right;
		} else if (child_left->right == 0) {
			node->value = child_left->value;
			if (child_left->left != 0)
				node->left += child_left->left;
			else
				node->left = 0;
			setup_height(node);
			return child_left;
		} else {
			struct node *moved_node = remove_from_left(child_right);

			node->value = moved_node->value;
			setup_height(node);
			return moved_node;
		}
	}
}

/*
 * Find the value in the subtree, remove it,
 * and return the address of the slot which became
 * free during removal - not necceseraly the slot
 * which held the value originally
 */
static struct node *
remove_under(struct node *cursor, uint64_t value)
{
	struct node *removed = NULL;

	assert(cursor->value != value);
	if (cursor->value < value && cursor->left != 0) {
		struct node *left = cursor + cursor->left;

		if (left->value == value) {
			removed = remove(left);
			if (removed == left)
				cursor->left = 0;
		} else {
			removed = remove_under(left, value);
		}
	} else if (cursor->value > value && cursor->right != 0) {
		struct node *right = cursor + cursor->right;

		if (right->value == value) {
			removed = remove(right);
			if (removed == right)
				cursor->right = 0;
		} else {
			removed = remove_under(right, value);
		}
	}

	if (removed != NULL) {
		rebalance(cursor);
		setup_height(cursor);
	}

	return removed;
}

void
uint64_avl_remove(struct uint64_avl *tree, uint64_t value)
{
	if (tree->count == 0)
		return;

	if (tree->count == 1) {
		if (first_node(tree)->value == value) {
			tree->data = shrink(tree->data, tree->memory_usage);
			tree->memory_usage = 0;
			tree->count = 0;
		}
		return;
	}

	struct node *removed;

	if (first_node(tree)->value == value) {
		removed = remove(first_node(tree));
	} else {
		removed = remove_under(tree->data, value);
	}

	if (removed != NULL) {
		if (removed != last_node(tree)) {
			assert(removed != first_node(tree));
			overwrite(removed, last_node(tree));
			reroute(tree->data, removed, last_node(tree));
		}
		tree->count--;

		size_t used_mem = (tree->count * sizeof (struct node));
		size_t unused_mem = tree->memory_usage - used_mem;

		if (unused_mem >= PAGESIZE) {
			tree->data = shrink(tree->data, tree->memory_usage);
			tree->memory_usage -= PAGESIZE;
		}
	}
}

static bool
is_in_subtree(const struct node *cursor, uint64_t value)
{
	if (cursor->value == value)
		return true;

	if (cursor->value < value) {
		if (cursor->left != 0)
			return is_in_subtree(cursor + cursor->left, value);
	} else {
		if (cursor->right != 0)
			return is_in_subtree(cursor + cursor->right, value);
	}

	return false;
}

bool
uint64_avl_containts(const struct uint64_avl *tree, uint64_t value)
{
	if (tree->count == 0)
		return false;

	return is_in_subtree(tree->data, value);
}
