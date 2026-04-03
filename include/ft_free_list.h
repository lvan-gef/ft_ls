#ifndef FT_FREE_LIST
#define FT_FREE_LIST

#include <stddef.h>

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

void free_list_free_all(free_list *fl);
void free_list_init(free_list *fl, void *data, size_t size);
free_list_node *free_list_find_best(free_list *fl, size_t size, size_t align,
                                    size_t *padding_,
                                    free_list_node **prev_node_);
void *free_list_alloc(free_list *fl, size_t size, size_t align);
void free_list_free(free_list *fl, void *ptr);
void free_list_coalescence(free_list *fl, free_list_node *prev_node,
                           free_list_node *free_node);
void free_list_node_insert(free_list_node **phead, free_list_node *prev_node,
                           free_list_node *new_node);
void free_list_node_remove(free_list_node **phead, free_list_node *prev_node,
                           free_list_node *del_node);

#endif // !FT_FREE_LIST
