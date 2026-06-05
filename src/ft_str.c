#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/ft_arena.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

t_str *init_str(const t_alloc *alloc, uint64_t cap) {
    if (cap > UINT64_MAX - 1 - sizeof(t_str)) {
        return NULL;
    }

    t_str *str = NULL;
    switch (alloc->kind) {
        case ALLOC_ARENA:
            str = arena_push(alloc->as.arena, sizeof(*str) + cap + 1);
            break;
        case ALLOC_FL:
            str = fl_alloc(alloc->as.fl, sizeof(*str) + cap + 1, 8);
            break;
    }

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

    ft_memcpy(new_str->str, str, len);
    new_str->len = len;
    new_str->str[new_str->len] = '\0';
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

    str->str[str->cap - 1] = '\0';
    str->pos = str->cap - 1;
    while (str->pos) {
        --str->pos;
        str->str[str->pos] = (char)((nbr % 10) + '0');
        nbr /= 10;
        ++str->len;
    }

    return str;
}

void free_str(free_list *fl, t_str *str) {
    if (!str) {
        return;
    }

    fl_free(fl, str);
}
