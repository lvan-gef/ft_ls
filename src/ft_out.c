#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

#include "./ft_out.h"
#include "./ft_utils.h"

void out_init(t_out *out, char *buf, const size_t cap, const int fd) {
    out->buf = buf;
    out->cap = cap;
    out->len = 0;
    out->fd = fd;
    out->failed = false;
}

void out_put(t_out *out, const char *src, size_t len) {
    while (len && !out->failed) {
        if (out->len == out->cap) {
            out_flush(out);
            continue;
        }

        const size_t avail = out->cap - out->len;
        const size_t n = len < avail ? len : avail;
        ft_memcpy(out->buf + out->len, src, n);
        out->len += n;
        src += n;
        len -= n;
    }
}

void out_str(t_out *out, const char *str) {
    out_put(out, str, ft_strlen(str));
}

void out_pad(t_out *out, const char c, size_t count) {
    while (count && !out->failed) {
        if (out->len == out->cap) {
            out_flush(out);
            continue;
        }

        const size_t avail = out->cap - out->len;
        const size_t n = count < avail ? count : avail;
        ft_memset(out->buf + out->len, c, n);
        out->len += n;
        count -= n;
    }
}

void out_flush(t_out *out) {
    size_t done = 0;

    while (!out->failed && done < out->len) {
        const ssize_t n = write(out->fd, out->buf + done, out->len - done);
        if (n > 0) {
            done += (size_t)n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            out->failed = true;
        }
    }

    out->len = 0;
}
