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

char *left_pad(Arena *arena, size_t nbr, size_t nbr_len) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(nbr_len, "nbr_len must be > 0");

    const size_t new_cap = nbr_len + 1;
    ASSERT_(new_cap > nbr_len, "new_cap did overflow");

    char *nbr_str = ArenaPush(arena, new_cap);
    if (!nbr_str) {
        return NULL;
    }

    size_t str_len = uitoa(nbr_str, new_cap, nbr);
    U64 arena_pos = ArenaPos(arena);
    char *buffer = ArenaPush(arena, new_cap);
    if (!buffer) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    size_t pad_count = (nbr_len > str_len) ? nbr_len - str_len : 0;
    size_t len = 0;
    while (len < pad_count) {
        buffer[len] = ' ';
        ++len;
    }

    len += ft_strlcpy(buffer + len, nbr_str, new_cap - len);
    ASSERT_(len == nbr_len, "len != nbr_len");

    ASSERT_(buffer, "buffer can not be NULL");
    ASSERT_(*buffer, "*buffer can not be '\\0'");
    return buffer;
}

char *rigth_pad(Arena *arena, t_str *str, size_t len) {
    ASSERT_(arena, "arena can not be NULL");

    const size_t new_cap = len + 1;
    ASSERT_(new_cap > len, "new_cap did overflow");

    char *new_str = ArenaPush(arena, new_cap);
    if (!new_str) {
        return NULL;
    }

    U64 arena_pos = ArenaPos(arena);
    char *buffer = ArenaPush(arena, new_cap);
    if (!buffer) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    size_t wb_len = 0;
    wb_len += ft_strlcpy(buffer, str->str, new_cap);

    while (wb_len < new_cap - 1) {
        buffer[wb_len] = ' ';
        ++wb_len;
    }
    ASSERT_(len == wb_len, "len != nbr_len");

    ASSERT_(buffer, "buffer can not be NULL");
    ASSERT_(*buffer, "*buffer can not be '\\0'");
    return buffer;
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
