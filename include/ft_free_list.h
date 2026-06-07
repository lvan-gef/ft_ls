#ifndef FT_FREE_LIST
#define FT_FREE_LIST

#include <stddef.h>

#ifndef FL_DEFAULT_SIZE
#define FL_DEFAULT_SIZE UINT64_C(4096)
#endif // !FL_DEFAULT_SIZE

typedef struct free_list_header free_list_header;
struct free_list_header {
    size_t block_size;
    size_t padding;
    void *allocation_base;
    free_list_header *next_extra;
};

typedef struct free_list_node free_list_node;
struct free_list_node {
    free_list_node *next;
    size_t block_size;
};

typedef struct free_list free_list;
struct free_list {
    void *data;
    size_t size;
    size_t used;

    free_list_node *head;
    free_list_header *extra_allocs;
};

typedef struct free_list_node_align_helper {
    char c;
    free_list_node member;
} free_list_node_align_helper;

enum { FREE_LIST_NODE_ALIGN = offsetof(free_list_node_align_helper, member) };

void fl_free_all(free_list *fl);
void fl_init(free_list *fl, void *data, size_t size);
void *fl_alloc(free_list *fl, size_t size);
void fl_free(free_list *fl, void *ptr);

#endif // !FT_FREE_LIST
