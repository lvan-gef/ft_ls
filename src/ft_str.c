#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/ft_arena.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static void *alloc_push_(const t_alloc *alloc, uint64_t size, uint64_t align);
static void to_uint_(t_str *str, uint64_t nbr);
static void fill_str_(t_str *dst, const char *src, uint64_t len);

t_str *init_str(const t_alloc *alloc, uint64_t cap) {
    if (cap > UINT64_MAX - 1 - sizeof(t_str)) {
        return NULL;
    }

    t_str *str = alloc_push_(alloc, sizeof(*str) + cap + 1, 8);
    if (!str) {
        return NULL;
    }

    str->str = (char *)(str + 1);
    str->cap = cap + 1;
    str->len = 0;
    str->pos = 0;
    str->str[0] = '\0';
    return str;
}


t_str *create_str(const t_alloc *alloc, const char *str) {
    const size_t len = ft_strlen(str);
    if (len + 1 < len) {
        return NULL;
    }

    t_str *new_str = init_str(alloc, len);
    if (!new_str) {
        return NULL;
    }

    fill_str_(new_str, str, len);
    return new_str;
}

t_str *dup_str(const t_alloc *alloc, const t_str *str) {
    t_str *new_str = init_str(alloc, str->cap - 1);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str->str + str->pos, str->len);
    new_str->str[str->len] = '\0';
    new_str->len = str->len;

    return new_str;
}

t_str *uint_to_str(const t_alloc *alloc, uint64_t nbr) {
    uint64_t len = len_of_nbr(nbr);
    t_str *str = init_str(alloc, len);
    if (!str) {
        return NULL;
    }

    to_uint_(str, nbr);
    return str;
}

void free_str(free_list *fl, t_str *str) {
    if (!str) {
        return;
    }

    fl_free(fl, str);
}

static void *alloc_push_(const t_alloc *alloc, uint64_t size, uint64_t align) {
    if (alloc->kind == ALLOC_ARENA) {
        (void)align;
        return arena_push(alloc->as.arena, size);
    }

    return fl_alloc(alloc->as.fl, size, align);
}

static void to_uint_(t_str *str, uint64_t nbr) {
    str->str[str->cap - 1] = '\0';
    str->pos = str->cap - 1;
    while (str->pos) {
        --str->pos;
        str->str[str->pos] = (char)((nbr % 10) + '0');
        nbr /= 10;
        ++str->len;
    }
}

static void fill_str_(t_str *dst, const char *src, uint64_t len) {
    ft_memcpy(dst->str, src, len);
    dst->len = len;
    dst->str[dst->len] = '\0';
}
