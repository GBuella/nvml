
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct uint64_avl {
	size_t count;
	size_t memory_usage;
	void *data;
};

void uint64_avl_init(struct uint64_avl *);
void uint64_avl_destroy(struct uint64_avl *);
void uint64_avl_insert(struct uint64_avl *, uint64_t);
void uint64_avl_remove(struct uint64_avl *, uint64_t);
bool uint64_avl_containts(const struct uint64_avl *, uint64_t);
