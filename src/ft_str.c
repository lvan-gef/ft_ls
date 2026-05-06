#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../include/ft_free_list.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"
#include "ft_arena.h"

static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize);

t_str *init_str(free_list *fl, uint64_t cap) {
    t_str *str = fl_alloc(fl, sizeof(*str), 8);
    if (!str) {
        goto failed;
    }

    if (cap + 1 < cap) {
        goto failed;
    }

    str->str = fl_alloc(fl, cap + 1, 8);
    if (!str->str) {
        goto failed;
    }

    str->cap = cap + 1;
    str->len = 0;
    str->pos = 0;

    return str;

failed:
    if (str) {
        free_str(fl, str);
    }

    return NULL;
}

t_str *create_str(free_list *fl, const char *str) {
    const size_t len = ft_strlen(str);
    if (len + 1 < len) {
        return NULL;
    }

    t_str *new_str = init_str(fl, len);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str, len);
    new_str->len = len;
    new_str->str[new_str->len] = '\0';

    return new_str;
}

t_str *dup_str(free_list *fl, const t_str *str) {
    t_str *new_str = init_str(fl, str->cap - 1);
    if (!new_str) {
        return NULL;
    }

    uint64_t len = strlcpy_(new_str, str, str->len + 1);
    new_str->len = len;

    return new_str;
}

t_str *dup_str_arena(arena *arena, const t_str *src) {
    t_str *dst = arena_push(arena, sizeof(*dst));
    char *buf;
    if (!dst) {
        return NULL;
    }

    buf = arena_push_no_zero(arena, src->cap);
    if (!buf) {
        return NULL;
    }

    *dst = *src;
    dst->str = buf;
    dst->pos = 0;
    ft_memcpy(dst->str, src->str, src->len + 1);
    return dst;
}

uint64_t cat_str(t_str *dst, const t_str *src) {
    if (dst->cap - dst->len == 0) {
        return 0;
    }

    uint64_t cur_pos = dst->pos;
    dst->pos = dst->len;
    uint64_t len = strlcpy_(dst, src, dst->cap - dst->len + 1);
    dst->len += len;
    dst->pos = cur_pos;

    return len;
}

t_str *uint_to_str(free_list *fl, uint64_t nbr) {
    uint64_t len = len_of_nbr(nbr);
    t_str *str = init_str(fl, len);
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

ssize_t append_chars_str(t_str *dst, const char *src) {
    const size_t src_len = ft_strlen(src);
    if ((uint64_t)src_len >= dst->cap - dst->len) {
        return -1;
    }

    ft_memcpy(dst->str + dst->len, src, src_len + 1);
    dst->len += (uint64_t)src_len;
    return (ssize_t)src_len;
}

bool has_next_str(const t_str *s) {
    return s->pos < s->len;
}

char peek_str(t_str *s) {
    return s->str[s->pos];
}

char next_str(t_str *s) {
    return s->str[s->pos++];
}

void free_str(free_list *fl, t_str *str) {
    if (str->str) {
        fl_free(fl, str->str);
    }

    fl_free(fl, str);
}

static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize) {
    if (!dstsize) {
        return 0;
    }

    uint64_t index = 0;
    uint64_t copied = 0;
    while (src->str[src->pos + index] && dst->pos + index < dst->cap) {
        if (index < dstsize - 1) {
            dst->str[dst->pos + index] = src->str[src->pos + index];
            copied = index + 1;
        }
        ++index;
    }

    dst->str[dst->pos + copied] = '\0';
    return copied;
}
