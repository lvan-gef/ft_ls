#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_str.h"

#include "ft_printer_helper.h"
#include "ft_shell_escape.h"

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

bool put_shell_escaped(t_str *out, const t_str *str, const char quote,
                       const bool pad_unquoted) {
    const uint64_t need = shell_escaped_len(str, quote, pad_unquoted);
    if (need > out->cap - 1) {
        if (quote == '\0') {
            return (!pad_unquoted || put_mem(out, " ", 1)) &&
                   put_mem(out, str->str + str->pos, str->len);
        }

        t_str *escaped = shell_escape_str(str, quote);
        if (!escaped) {
            return false;
        }

        const bool ok = put_mem(out, escaped->str + escaped->pos, escaped->len);
        str_free(escaped);
        return ok;
    }

    if (out->cap - 1 - out->len < need && !flush_str(out)) {
        return false;
    }

    return shell_escape_append(out, str, quote, pad_unquoted);
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

bool put_dir_header(t_str *out, const t_str *dir_header) {
    if (!dir_header) {
        return true;
    }

    t_shell_scan scan;
    shell_scan_str(dir_header, &scan);
    if (scan.quote == '\0' && ft_memchr(dir_header->str + dir_header->pos, ':',
                                        (size_t)dir_header->len) != NULL) {
        scan.quote = '\'';
    }

    return put_shell_escaped(out, dir_header, scan.quote, false) &&
           put_mem(out, ":\n", 2);
}

bool context_needs_padding(const t_array *context) {
    if (!context) {
        return false;
    }

    for (uint64_t index = 0; index < context->len; ++index) {
        const t_str *str = context->data[index];
        if (!str) {
            continue;
        }

        t_shell_scan scan;
        shell_scan_str(str, &scan);
        if (scan.quote != '\0') {
            return true;
        }
    }

    return false;
}
