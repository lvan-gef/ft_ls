#include <stddef.h>

#include "../include/ft_helpers.h"
#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"

#include "../libft/include/libft.h"

static size_t get_len_(size_t c, size_t base);

size_t uitoa(char *buffer, size_t buffer_len, size_t n) {
    size_t base = 10;
    size_t len = get_len_(n, base);

    if (len > buffer_len - 1) {
        len = buffer_len - 1;
    }

    size_t counter = 0;
    if (n == 0) {
        buffer[0] = '0';
        ++counter;
    }

    while (n) {
        buffer[len - 1] = (char)((n % 10) + '0');
        n = n / 10;
        --len;
        ++counter;
    }

    return counter;
}

size_t get_len(size_t n) {
    return get_len_(n, 10);

}

t_str *join_paths(Arena *arena, t_str *path, t_str *filename) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->len, "path->len must be > 0");
    ASSERT_(*path->str, "*path->str can not be '\\0'");
    ASSERT_(filename, "filename can not be NULL)");
    ASSERT_(filename->len, "filename->len must be > 0");
    ASSERT_(*filename->str, "*filename->str can not be '\\0'");

    const U64 arena_pos = ArenaPos(arena);
    t_str *new_path = ArenaPush(arena, sizeof(*new_path));
    if (!new_path) {
        return NULL;
    }

    const size_t new_size = path->len + 1 + filename->len + 1;
    new_path->str = ArenaPush(arena, new_size);
    if (!new_path->str) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    size_t len = ft_strlcpy(new_path->str, path->str, new_size);

    ASSERT_(len - 1 < len, "len did underflow");
    if (new_path->str[len - 1] != '/') {
        len += ft_strlcpy(new_path->str + len, "/", new_size - len);
    }
    len += ft_strlcpy(new_path->str + len, filename->str, new_size - len);

    new_path->len = len;
    new_path->cap = new_size - 1;

    return new_path;
}

static size_t get_len_(size_t c, size_t base) {
    size_t counter = 0;

    if (c == 0) {
        ++counter;
    }

    while (c) {
        c = c / base;
        ++counter;
    }

    return counter;
}
