
#include <stdint.h>
#include "../include/ft_str.h"
#include "../include/ft_assert.h"
#include "../libft/include/libft.h"
#include "../libft/include/ft_fprintf.h"

t_str *init_str(Arena *arena, uint64_t cap) {
    ASSERT_NOTNULL(arena);
    ASSERT_GT(cap, 0);


    char *err_msg = NULL;
    const uint64_t arena_pos = ArenaPos(arena);
    t_str *str = ArenaPushNoZero(arena, sizeof(*str));
    if (!str) {
        err_msg = (char *)"ArenaPushNoZero failed";
        goto failed;
    }

    ASSERT_GT(cap + 1, cap);
    if (cap + 1 < cap) {
        err_msg = (char *)"cap did overflow";
        goto failed;
    }

    str->str = ArenaPushNoZero(arena, cap + 1);
    if (!str->str) {
        err_msg = (char *)"ArenaPushNoZero failed";
        goto failed;
    }

    str->cap = cap;
    str->len = 0;

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
    ASSERT_GT(len + 1, len);
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

    ASSERT_NOTNULL(new_str);
    ASSERT_GT(new_str->cap, 1);
    ASSERT_GT(new_str->len, 1);
    ASSERT_EQ(new_str->len, new_str->cap);
    ASSERT_EQ(new_str->str, str);
    return new_str;
}

t_str *dup_str(Arena *arena, const t_str *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);
    ASSERT_GT(str->cap, 1);
    ASSERT_GT(str->len, 1);
    ASSERT_(*str->str, "%c can not be '\\0'", *str->str);

    t_str *new_str = init_str(arena, str->cap);
    if (!new_str) {
        return NULL;
    }

    ft_memcpy(new_str->str, str->str, str->len);
    new_str->str[new_str->len] = '\0';
    new_str->len = str->len;

    ASSERT_NOTNULL(new_str);
    ASSERT_EQ(new_str->str, str->str);
    ASSERT_EQ(new_str->cap, str->cap);
    ASSERT_EQ(new_str->len, str->len);

    return new_str;
}

uint64_t join_str(t_str *dst, const t_str *src) {
    ASSERT_NOTNULL(dst);
    ASSERT_GT(dst->cap, 1);
    ASSERT_NOTNULL(src);
    ASSERT_GT(src->cap, 1);
    ASSERT_GT(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);

    if (dst->cap - dst->len == 0) {
        return 0;
    }

    return ft_strlcpy(dst->str + dst->len, src->str, dst->cap - dst->len + 1);
}

uint64_t join_l_str(t_str *dst, const t_str *src, uint64_t size) {
    ASSERT_NOTNULL(dst);
    ASSERT_GT(dst->cap, 1);
    ASSERT_NOTNULL(src);
    ASSERT_GT(src->cap, 1);
    ASSERT_GT(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);
    ASSERT_GT(size, 0);

    uint64_t cap = dst->cap - dst->len;
    if (!cap) {
        return 0;
    }

    if (size > cap) {
        size = cap;
    }

    return ft_strlcpy(dst->str + dst->len, src->str, dst->cap - dst->len + 1);
}

uint64_t cpy_str(t_str *dst, const t_str *src) {
    ASSERT_NOTNULL(dst);
    ASSERT_GT(dst->cap, 1);
    ASSERT_EQ(dst->len, 0);
    ASSERT_(!*dst->str, "%c can not be '\\0'", *dst->str);
    ASSERT_NOTNULL(src);
    ASSERT_GT(src->cap, 1);
    ASSERT_GT(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);
    ASSERT_EQ(dst->cap, src->cap);

    return ft_strlcpy(dst->str, src->str, dst->cap + 1);
}
uint64_t cpy_l_str(t_str *dst, const t_str *src, uint64_t size) {
    ASSERT_NOTNULL(dst);
    ASSERT_GT(dst->cap, 1);
    ASSERT_EQ(dst->len, 0);
    ASSERT_(!*dst->str, "%c can not be '\\0'", *dst->str);
    ASSERT_NOTNULL(src);
    ASSERT_GT(src->cap, 1);
    ASSERT_GT(src->len, 1);
    ASSERT_(*src->str, "%c can not be '\\0'", *src->str);

    return ft_strlcpy(dst->str, src->str, size);
}
