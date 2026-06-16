#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_printer_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

bool put_mem(t_str *out, const char *src, uint64_t len) {
    while (len) {
        if (out->len == out->cap - 1 && !flush_str(out)) {
            return false;
        }

        const uint64_t avail = (out->cap - 1) - out->len;
        const uint64_t to_copy = len < avail ? len : avail;
        ft_memcpy(out->str + out->len, src, (size_t)to_copy);
        out->len += to_copy;
        out->str[out->len] = '\0';
        src += to_copy;
        len -= to_copy;
    }

    return true;
}

bool flush_str(t_str *out) {
    if (!out->len) {
        return true;
    }

    uint64_t written_total = 0;
    while (written_total < out->len) {
        const ssize_t written = write(STDOUT_FILENO, out->str + written_total,
                                      (size_t)(out->len - written_total));

        if (written > 0) {
            written_total += (uint64_t)written;
            continue;
        }
        if (written < 0 && errno == EINTR) {
            continue;
        }

        if (written_total > 0) {
            const uint64_t remaining = out->len - written_total;
            ft_memmove(out->str, out->str + written_total, (size_t)remaining);
            out->len = remaining;
            out->str[remaining] = '\0';
        }
        return false;
    }

    out->len = 0;
    out->str[0] = '\0';
    return true;
}
