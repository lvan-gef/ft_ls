#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/ft_free_list.h"

static size_t calc_padding_with_header(uintptr_t ptr, uintptr_t align,
                                       size_t header_size);
static bool is_power_of_two(uintptr_t x);
static size_t align_up(size_t x, size_t a);
static void free_extra_allocs_(free_list *fl);
static void *alloc_extra_block_(free_list *fl, size_t size, size_t align);
static void free_extra_block_(free_list *fl, free_list_header *header);

void free_list_free_all(free_list *fl) {
    free_extra_allocs_(fl);
    fl->used = 0;
    free_list_node *first_node = (free_list_node *)fl->data;
    first_node->block_size = fl->size;
    first_node->next = NULL;
    fl->head = first_node;
}

void free_list_init(free_list *fl, void *data, size_t size) {
    fl->data = data;
    fl->size = size;
    fl->extra_allocs = NULL;
    free_list_free_all(fl);
}

free_list_node *free_list_find_best(free_list *fl, size_t size, size_t align,
                                    size_t *padding_,
                                    free_list_node **prev_node_) {
    size_t smallest_diff = ~(size_t)0;
    free_list_node *node = fl->head;
    free_list_node *prev_node = NULL;
    free_list_node *best_node = NULL;
    free_list_node *best_prev_node = NULL;
    size_t best_padding = 0;
    while (node != NULL) {
        size_t padding = calc_padding_with_header((uintptr_t)node, align,
                                                  sizeof(free_list_header));
        size_t required_space = size + padding;
        if (node->block_size >= required_space) {
            size_t diff = node->block_size - required_space;
            if (diff < smallest_diff) {
                smallest_diff = diff;
                best_node = node;
                best_prev_node = prev_node;
                best_padding = padding;
            }
        }
        prev_node = node;
        node = node->next;
    }

    if (padding_) {
        *padding_ = best_padding;
    }

    if (prev_node_) {
        *prev_node_ = best_prev_node;
    }

    return best_node;
}

void *free_list_alloc(free_list *fl, size_t size, size_t align) {
    size_t padding = 0;
    free_list_node *prev_node = NULL;
    free_list_node *node = NULL;
    size_t align_padding, required_space, remaining;
    free_list_header *header_ptr;

    if (size < sizeof(free_list_node)) {
        size = sizeof(free_list_node);
    }

    if (align < 8) {
        align = 8;
    }

    assert(is_power_of_two(align));
    if (!is_power_of_two(align)) {
        return NULL;
    }

    node = free_list_find_best(fl, size, align, &padding, &prev_node);
    if (node == NULL) {
        void *extra = alloc_extra_block_(fl, size, align);
        if (extra != NULL) {
            return extra;
        }
        assert(0 && "Free list has no free memory");
        return NULL;
    }

    align_padding = padding - sizeof(free_list_header);
    required_space = size + padding;
    required_space = align_up(required_space, FREE_LIST_NODE_ALIGN);
    remaining = node->block_size - required_space;
    if (remaining >= sizeof(free_list_node)) {
        uintptr_t new_addr = (uintptr_t)node + required_space;
        assert((new_addr & (FREE_LIST_NODE_ALIGN - 1)) == 0);
        free_list_node *new_node = (free_list_node *)new_addr;
        new_node->block_size = remaining;
        free_list_node_insert(&fl->head, node, new_node);
    } else {
        required_space = node->block_size;
    }

    free_list_node_remove(&fl->head, prev_node, node);
    header_ptr = (free_list_header *)((uintptr_t)node + align_padding);
    header_ptr->block_size = required_space;
    header_ptr->padding = align_padding;
    fl->used += required_space;

    return (void *)((char *)header_ptr + sizeof(free_list_header));
}

void free_list_free(free_list *fl, void *ptr) {
    free_list_header *header;
    free_list_node *free_node;
    free_list_node *node;
    free_list_node *prev_node = NULL;
    if (ptr == NULL) {
        return;
    }

    header = (free_list_header *)((uintptr_t)ptr - sizeof(free_list_header));
    if (header->allocation_base != NULL) {
        free_extra_block_(fl, header);
        return;
    }

    free_node = (free_list_node *)((uintptr_t)header - header->padding);
    free_node->block_size = header->block_size;
    free_node->next = NULL;
    node = fl->head;
    while (node != NULL) {
        if ((void *)free_node < (void *)node) {
            free_list_node_insert(&fl->head, prev_node, free_node);
            break;
        }
        prev_node = node;
        node = node->next;
    }

    if (node == NULL) {
        free_list_node_insert(&fl->head, prev_node, free_node);
    }

    fl->used -= free_node->block_size;
    free_list_coalescence(fl, prev_node, free_node);
}

void free_list_coalescence(free_list *fl, free_list_node *prev_node,
                           free_list_node *free_node) {
    if (free_node->next != NULL &&
        (void *)((char *)free_node + free_node->block_size) ==
            (void *)free_node->next) {
        free_node->block_size += free_node->next->block_size;
        free_list_node_remove(&fl->head, free_node, free_node->next);
    }

    if (prev_node != NULL &&
        (void *)((char *)prev_node + prev_node->block_size) ==
            (void *)free_node) {
        prev_node->block_size += free_node->block_size;
        free_list_node_remove(&fl->head, prev_node, free_node);
    }
}

void free_list_node_insert(free_list_node **phead, free_list_node *prev_node,
                           free_list_node *new_node) {
    if (prev_node == NULL) {
        new_node->next = *phead;
        *phead = new_node;
    } else {
        new_node->next = prev_node->next;
        prev_node->next = new_node;
    }
}

void free_list_node_remove(free_list_node **phead, free_list_node *prev_node,
                           free_list_node *del_node) {
    if (prev_node == NULL) {
        *phead = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
}

static size_t calc_padding_with_header(uintptr_t ptr, uintptr_t align,
                                       size_t header_size) {
    assert(is_power_of_two(align));
    if (!is_power_of_two(align)) {
        return 0;
    }

    uintptr_t p = ptr;
    uintptr_t a = align;
    uintptr_t modulo = p & (a - 1);
    uintptr_t padding = 0;
    if (modulo != 0) {
        padding = a - modulo;
    }

    uintptr_t needed_space = (uintptr_t)header_size;
    if (padding < needed_space) {
        needed_space -= padding;
        if ((needed_space & (a - 1)) != 0) {
            padding += a * (1 + (needed_space / a));
        } else {
            padding += a * (needed_space / a);
        }
    }
    return (size_t)padding;
}

static bool is_power_of_two(uintptr_t x) {
    return (x & (x - 1)) == 0;
}

static size_t align_up(size_t x, size_t a) {
    return (x + (a - 1)) & ~(a - 1);
}

static void free_extra_allocs_(free_list *fl) {
    free_list_header *header = fl->extra_allocs;
    while (header != NULL) {
        free_list_header *next = header->next_extra;
        free(header->allocation_base);
        header = next;
    }

    fl->extra_allocs = NULL;
}

static void *alloc_extra_block_(free_list *fl, size_t size, size_t align) {
    if (size > SIZE_MAX - sizeof(free_list_header) - (align - 1)) {
        return NULL;
    }

    const size_t total_size = size + sizeof(free_list_header) + (align - 1);
    void *base = malloc(total_size);
    if (base == NULL) {
        return NULL;
    }

    const uintptr_t payload_addr =
        align_up((size_t)((uintptr_t)base + sizeof(free_list_header)), align);
    free_list_header *header =
        (free_list_header *)(payload_addr - sizeof(free_list_header));
    header->block_size = total_size;
    header->padding = 0;
    header->allocation_base = base;
    header->next_extra = fl->extra_allocs;
    fl->extra_allocs = header;
    fl->used += total_size;
    return (void *)payload_addr;
}

static void free_extra_block_(free_list *fl, free_list_header *header) {
    free_list_header **slot = &fl->extra_allocs;
    while (*slot != NULL && *slot != header) {
        slot = &(*slot)->next_extra;
    }

    assert(*slot == header);
    if (*slot != header) {
        return;
    }

    *slot = header->next_extra;
    fl->used -= header->block_size;
    free(header->allocation_base);
}
