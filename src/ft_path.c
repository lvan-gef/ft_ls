#include <stdint.h>

#include "../include/ft_path.h"
#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"


t_str *get_path_entry(Arena *arena, t_entry *entry) {
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->path);
    ASSERT_GT(entry->path->cap, 0);
    ASSERT_LT(entry->path->len, entry->path->cap);
    ASSERT_NOTNULL(arena);

    const uint64_t arena_pos = ArenaPos(arena);
    char *str = ArenaPush(arena, entry->path->len);
    if (!str) {
        goto failed;
    }
    ft_strlcpy(str, entry->path->str, entry->path->cap);

    char *last_slash = ft_strrchr(str, '/');
    if (!last_slash) {
        goto failed;
    }

    long len = last_slash - str;
    str[len] = '\0';

    t_str *new_str = create_str(arena, str);
    if (!new_str) {
        goto failed;
    }

    return new_str;

failed:
    if (str) {
        ArenaPopTo(arena, arena_pos);
    }

    return NULL;
}
