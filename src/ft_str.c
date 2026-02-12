#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_assert.h"
#include "../include/ft_str.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

// TODO: replace all ft_strlcpy it use strlen...
static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize);

t_str *init_str(Arena *arena, uint64_t cap) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(cap, 0);

    const char *err_msg = NULL;
    const uint64_t arena_pos = ArenaPos(arena);
    t_str *str = ArenaPushNoZero(arena, sizeof(*str));
    if (!str) {
        err_msg = "ArenaPushNoZero failed";
        goto failed;
    }

    if (cap + 1 < cap) {
        err_msg = "cap did overflow";
        goto failed;
    }

    str->str = ArenaPushNoZero(arena, cap + 1);
    if (!str->str) {
        err_msg = "ArenaPushNoZero failed";
        goto failed;
    }

    str->cap = cap;
    str->len = 0;
    str->pos = 0;

    ASSERT_NOTNULL(str);
    return str;

failed:
    ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    ArenaPopTo(arena, arena_pos);
    return NULL;
}

t_str *create_str(Arena *arena, const char *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);
    ASSERT_(*str, "%c can not be '\\0'", *str);

    const size_t len = ft_strlen(str);
    if (len + 1 < len) {
        ft_fprintf(STDERR_FILENO, "new cap did underflow");
        return NULL;
    }

    t_str *new_str = init_str(arena, len + 1);
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

t_str *dup_str(Arena *arena, const t_str *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);
    ASSERT_GE(str->cap, 2);
    ASSERT_GE(str->len, 1);
    ASSERT_(*str->str, "%c can not be '\\0'", *str->str);

    t_str *new_str = init_str(arena, str->cap);
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
    size_t len = strlcpy_(dst, src, dst->cap - dst->len + 1);
    dst->len += len;
    dst->pos = cur_pos;
    return len;
}

uint64_t cat_l_str(t_str *dst, const t_str *src, uint64_t size) {
    ASSERT_NOTNULL(dst);
    ASSERT_GE(dst->cap, 2);
    ASSERT_NOTNULL(src);
    ASSERT_GE(src->cap, 2);
    ASSERT_GE(src->len, 1);
    ASSERT_LT(src->pos, src->len);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);
    ASSERT_GT(size, 0);

    uint64_t cap = dst->cap - dst->len;
    if (!cap) {
        return 0;
    }

    if (size > cap) {
        size = cap;
    }

    uint64_t cur_pos = dst->pos;
    dst->pos = dst->len;

    uint64_t len = strlcpy_(dst, src, size + 1);
    dst->len += len;
    dst->pos = cur_pos;
    return len;
}

uint64_t cpy_str(t_str *dst, const t_str *src) {
    ASSERT_NOTNULL(dst);
    ASSERT_GE(dst->cap, 2);
    ASSERT_EQ(dst->len, 0);
    ASSERT_(!*dst->str, "%c can not be '\\0'", *dst->str);
    ASSERT_NOTNULL(src);
    ASSERT_GE(src->cap, 2);
    ASSERT_GE(src->len, 1);
    ASSERT_LT(src->pos, src->len);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);
    ASSERT_EQ(dst->cap, src->cap);

    return strlcpy_(dst, src, dst->cap + 1);
}
uint64_t cpy_l_str(t_str *dst, const t_str *src, uint64_t size) {
    ASSERT_NOTNULL(dst);
    ASSERT_GE(dst->cap, 2);
    ASSERT_EQ(dst->len, 0);
    ASSERT_(!*dst->str, "%c can not be '\\0'", *dst->str);
    ASSERT_NOTNULL(src);
    ASSERT_GE(src->cap, 2);
    ASSERT_GE(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);

    return strlcpy_(dst, src, size);
}

bool has_next_str(t_str *s) {
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

static uint64_t strlcpy_(t_str *dst, const t_str *src, uint64_t dstsize) {
    ASSERT_LT(src->pos, src->cap);

    if (!dstsize) {
        return (src->len);
    }

    uint64_t index = 0;
    while (src->str[src->pos + index]) {
        if (index < dstsize - 1) {
            dst->str[dst->pos + index] = src->str[src->pos + index];
        }
        ++index;
    }

    dst->str[dst->pos + index] = '\0';
    return src->len;
}
