#include "../include/ft_ls.h"
#include "../include/ft_arena.h"
#include "../libft/include/libft.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"

t_path *init_path(Arena *arena) {
    ASSERT_(arena, "arena can not be NULL");
    const U64 arena_pos = ArenaPos(arena);
    t_path *path = ArenaPush(arena, sizeof(*path));
    if (!path) {
        return NULL;
    }

    path->paths = init_array(arena, DEFAULT_SIZE, ARRAY_PATHS);
    if (!path->paths) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    return path;
}

t_file *init_file(Arena *arena) {
    ASSERT_(arena, "arena can not be NULL");
    t_file *file = ArenaPush(arena, sizeof(*file));
    if (!file) {
        return NULL;
    }

    return file;
}

t_str *create_str(Arena *arena, const char *str) {
    ASSERT_(arena, "arena can not be NULL");
    ASSERT_(str, "str can not be NULL");
    ASSERT_(*str, "*str can not be '\\0'");

    const U64 arena_pos = ArenaPos(arena);
    t_str *new_str = ArenaPush(arena, sizeof(*new_str));
    if (!str) {
        return NULL;
    }

    const size_t len = ft_strlen(str);
    new_str->str = ArenaPush(arena, len + 1);
    if (!new_str) {
        ArenaPopTo(arena, arena_pos);
        return NULL;
    }

    ft_strlcpy(new_str->str, str, len + 1);
    new_str->cap = len;
    new_str->len = len;

    return new_str;
}
