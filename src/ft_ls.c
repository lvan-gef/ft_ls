#include "../include/ft_ls.h"
#include "../include/ft_arena.h"
#include "../libft/include/libft.h"

t_path *init_path(Arena *arena) {
    t_path *path = ArenaPush(arena, sizeof(*path));
    if (!path) {
        return NULL;
    }

    return path;
}

t_file *init_file(Arena *arena) {
    t_file *file = ArenaPush(arena, sizeof(*file));
    if (!file) {
        return NULL;
    }

    return file;
}

t_str *create_str(Arena *arena, const char *str) {
    t_str *new_str = ArenaPush(arena, sizeof(*new_str));
    if (!str) {
        return NULL;
    }

    const size_t len = ft_strlen(str);
    new_str->str = ArenaPush(arena, len + 1);
    if (!str) {
        return NULL;
    }

    ft_strlcpy(new_str->str, str, len + 1);
    new_str->cap = len;
    new_str->len = len;

    return new_str;
}
