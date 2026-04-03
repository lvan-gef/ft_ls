#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/ft_assert.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../include/ft_free_list.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize);

t_str *init_str(free_list *fl, uint64_t cap) {
    ASSERT_NOTNULL(fl);
    ASSERT_GT(cap, 0);

    const char *err_msg = NULL;
    t_str *str = free_list_alloc(fl, sizeof(*str), 8);
    if (!str) {
        err_msg = "ArenaPushNoZero failed";
        goto failed;
    }

    if (cap + 1 < cap) {
        err_msg = "cap did overflow";
        goto failed;
    }

    str->str = free_list_alloc(fl, cap + 1, 8);
    if (!str->str) {
        err_msg = "ArenaPushNoZero failed";
        goto failed;
    }

    str->cap = cap + 1;
    str->len = 0;
    str->pos = 0;

    ASSERT_NOTNULL(str);
    return str;

failed:
    ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    if (str) {
        free_str(fl, str);
    }

    return NULL;
}

t_str *create_str(free_list *fl, const char *str) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(str);
    ASSERT_(*str, "%c can not be '\\0'", *str);

    const size_t len = ft_strlen(str);
    if (len + 1 < len) {
        ft_fprintf(STDERR_FILENO, "new cap did underflow");
        return NULL;
    }

    t_str *new_str = init_str(fl, len + 1);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str, len);
    new_str->len = len;
    new_str->str[new_str->len] = '\0';

    ASSERT_NOTNULL(new_str);
    ASSERT_GE(new_str->cap, 2);
    ASSERT_GE(new_str->len, 1);
    ASSERT_LT(new_str->len, new_str->cap);
    return new_str;
}

t_str *dup_str(free_list *fl, const t_str *str) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(str);
    ASSERT_GE(str->cap, 2);
    ASSERT_GE(str->len, 1);
    ASSERT_(*str->str, "%c can not be '\\0'", *str->str);

    t_str *new_str = init_str(fl, str->cap - 1);
    if (!new_str) {
        return NULL;
    }

    uint64_t len = strlcpy_(new_str, str, str->len + 1);
    ASSERT_EQ(len, str->len);
    new_str->len = len;

    ASSERT_NOTNULL(new_str);
    ASSERT_EQ(new_str->cap, str->cap);
    ASSERT_EQ(new_str->len, str->len);

    return new_str;
}

uint64_t cat_str(t_str *dst, const t_str *src) {
    ASSERT_NOTNULL(dst);
    ASSERT_GE(dst->cap, 2);
    ASSERT_NOTNULL(src);
    ASSERT_GE(src->cap, 2);
    ASSERT_GE(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);

    if (dst->cap - dst->len == 0) {
        return 0;
    }

    uint64_t cur_pos = dst->pos;
    dst->pos = dst->len;
    uint64_t len = strlcpy_(dst, src, dst->cap - dst->len + 1);
    dst->len += len;
    dst->pos = cur_pos;

    ASSERT_LT(dst->len, dst->cap);
    ASSERT_LE(dst->pos, dst->len);
    return len;
}

t_str *uint_to_str(free_list *fl, uint64_t nbr) {
    ASSERT_NOTNULL(fl);

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

    ASSERT_NOTNULL(str);
    ASSERT_LT(str->len, str->cap);
    ASSERT_EQ(str->pos, 0);
    return str;
}

ssize_t append_chars_str(t_str *dst, const char *src) {
    ASSERT_NOTNULL(dst);
    ASSERT_NOTNULL(src);
    ASSERT_(*src, "%c can not be '\\0'", *src);

    const size_t src_len = ft_strlen(src);
    if ((uint64_t)src_len >= dst->cap - dst->len) {
        return -1;
    }

    ft_memcpy(dst->str + dst->len, src, src_len + 1);
    dst->len += (uint64_t)src_len;
    return (ssize_t)src_len;
}

bool has_next_str(const t_str *s) {
    ASSERT_NOTNULL(s);

    return s->pos < s->len;
}

char peek_str(t_str *s) {
    ASSERT_NOTNULL(s);

    return s->str[s->pos];
}

char next_str(t_str *s) {
    ASSERT_NOTNULL(s);

    return s->str[s->pos++];
}

void free_str(free_list *fl, t_str *str) {
    ASSERT_NOTNULL(str);

    if (str->str) {
        free_list_free(fl, str->str);
    }

    free_list_free(fl, str);
}

static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize) {
    ASSERT_LT(src->pos, src->cap);
    ASSERT_LT(dst->pos, dst->cap);
    ASSERT_GT(dstsize, 0);

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
