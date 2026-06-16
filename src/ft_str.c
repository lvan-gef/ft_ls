#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"
#include "../include/ft_arena.h"

#include "../libft/include/libft.h"

static void fill_init_str_(t_str *str, uint64_t cap);

t_str *arena_init_str(Arena *arena, const uint64_t cap) {
    if (cap > UINT64_MAX - 1 - sizeof(t_str)) {
        return NULL;
    }

    t_str *str = arena_push(arena, sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    fill_init_str_(str, cap);
    return str;
}

t_str *fl_init_str(free_list *fl, const uint64_t cap) {
    if (cap > UINT64_MAX - 1 - sizeof(t_str)) {
        return NULL;
    }

    t_str *str = fl_alloc(fl, sizeof(*str) + cap + 1);
    if (!str) {
        return NULL;
    }

    fill_init_str_(str, cap);
    return str;
}

t_str *create_str(free_list *fl, const char *str) {
    const size_t len = ft_strlen(str);
    if (len + 1 < len) {
        return NULL;
    }

    t_str *new_str = fl_alloc(fl, len);
    if (!str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str, len);
    new_str->len = len;
    new_str->str[new_str->len] = '\0';
    return new_str;
}

t_str *dup_str(free_list *fl, const t_str *str) {
    t_str *new_str = fl_alloc(fl, str->cap - 1);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str->str + str->pos, str->len);
    new_str->str[str->len] = '\0';
    new_str->len = str->len;

    return new_str;
}

t_str *uint_to_str(free_list *fl, uint64_t nbr) {
    const uint64_t len = len_of_nbr(nbr);
    t_str *str = fl_init_str(fl, len);
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
    fl_free(fl, str);
}

static void fill_init_str_(t_str *str, const uint64_t cap) {
    str->str = (char *)(str + 1);
    str->cap = cap + 1;
    str->len = 0;
    str->pos = 0;
    str->str[0] = '\0';
}
