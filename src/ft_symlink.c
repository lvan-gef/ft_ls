#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "./ft_arena.h"
#include "./ft_symlink.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif /* ifndef PATH_MAX */

t_str *read_symlink(t_arena *scratch, const t_str *path,
                    const size_t target_size, int *read_err) {
    if (read_err) {
        *read_err = 0;
    }

    size_t cap = target_size > 0 ? target_size + 1 : PATH_MAX;
    while (true) {
        const size_t read_size = (size_t)cap;
        t_str *str = str_arena_new(scratch, read_size);
        if (!str) {
            return NULL;
        }

        const ssize_t len = readlink(path->str, str->str, read_size);
        if (len < 0) {
            const int err = errno;
            if (read_err) {
                *read_err = err;
            }

            if (err == ENOENT || err == EINVAL || err == EACCES ||
                err == EPERM) {
                return str_arena_new(scratch, 0);
            }

            return NULL;
        }

        const size_t read_len = (size_t)len;
        if (read_len < cap) {
            str->len = read_len;
            str->str[read_len] = '\0';
            return str;
        }

        cap *= 2;
    }
}
