#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "./ft_arena.h"
#include "./ft_str_arena.h"
#include "./ft_symlink.h"

#ifndef PATH_MAX
#define PATH_MAX UINT64_C(4096)
#endif /* ifndef PATH_MAX */

t_str *read_symlink(Arena *scratch, const t_str *path,
                    const uint64_t target_size, int *read_err) {
    if (read_err) {
        *read_err = 0;
    }

    if (target_size == UINT64_MAX) {
        return NULL;
    }

    const uint64_t max_cap =
        (uint64_t)PTRDIFF_MAX - (uint64_t)sizeof(t_str) - 1;
    uint64_t cap = target_size > 0 ? target_size + 1 : PATH_MAX;
    while (true) {
        if (cap > max_cap) {
            return NULL;
        }

        const size_t read_size = (size_t)cap;
        const Arena_Mark mark = arena_get_mark(scratch);
        t_str *str = str_arena_new(scratch, read_size);
        if (!str) {
            return NULL;
        }

        const ssize_t len = readlink(path->str, str->str, read_size);
        if (len < 0) {
            const int err = errno;
            arena_pop_to_mark(scratch, mark);
            if (read_err) {
                *read_err = err;
            }

            if (err == ENOENT || err == EINVAL || err == EACCES ||
                err == EPERM) {
                return str_arena_new(scratch, 0);
            }

            return NULL;
        }

        const uint64_t read_len = (uint64_t)len;
        if (read_len < cap) {
            if (read_len >= str->cap) {
                arena_pop_to_mark(scratch, mark);
                return NULL;
            }

            str->len = read_len;
            str->str[read_len] = '\0';
            return str;
        }

        arena_pop_to_mark(scratch, mark);
        if (cap > max_cap / UINT64_C(2)) {
            return NULL;
        }

        cap *= 2;
    }
}
