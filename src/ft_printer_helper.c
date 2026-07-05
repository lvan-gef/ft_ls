#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/ft_str.h"

#include "./ft_utils.h"

#include "./ft_ls.h"
#include "./ft_printer_helper.h"
#include "./ft_shell_escape.h"

static bool put_shell_escaped_scan_(t_str *out, const t_str *str,
                                    const t_shell_scan *scan,
                                    bool pad_unquoted);

bool put_mem(t_str *out, const char *src, const uint64_t len) {
    return put_mem_fd(out, src, len, STDOUT_FILENO);
}

bool put_mem_fd(t_str *out, const char *src, uint64_t len, const int fd) {
    while (len) {
        if (out->len == out->cap - 1 && !flush_fd(out, fd)) {
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

bool put_entry_name(t_str *out, const t_entry *entry, const bool pad_unquoted) {
    const t_str *name = entry->name ? entry->name : entry->path;

    return put_shell_escaped_scan_(out, name, &entry->name_scan, pad_unquoted);
}

bool flush_fd(t_str *out, const int fd) {
    if (!out->len) {
        return true;
    }

    uint64_t written_total = 0;
    while (written_total < out->len) {
        const ssize_t written = write(fd, out->str + written_total,
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
            ft_memcpy(out->str, out->str + written_total, (size_t)remaining);
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
    if (scan.quote == '\0' &&
        ft_memchr(dir_header->str, ':', (size_t)dir_header->len) != NULL) {
        scan.quote = '\'';
    }

    const uint64_t need = shell_escaped_len(dir_header, scan.quote, false);

    return put_shell_escaped_scan_(out, dir_header,
                                   &(t_shell_scan){.display_len = need,
                                                   .padded_display_len = need,
                                                   .quote = scan.quote},
                                   false) &&
           put_mem(out, ":\n", 2);
}

static bool put_shell_escaped_scan_(t_str *out, const t_str *str,
                                    const t_shell_scan *scan,
                                    const bool pad_unquoted) {
    const uint64_t need =
        pad_unquoted ? scan->padded_display_len : scan->display_len;

    if (need > out->cap - 1) {
        if (!scan->quote) {
            return (!pad_unquoted || put_mem(out, " ", 1)) &&
                   put_mem(out, str->str, str->len);
        }

        t_str *escaped = shell_escape_str(str, scan->quote);
        if (!escaped) {
            return false;
        }

        const bool ok = put_mem(out, escaped->str, escaped->len);
        str_free(escaped);
        return ok;
    }

    if (out->cap - 1 - out->len < need && !flush_fd(out, STDOUT_FILENO)) {
        return false;
    }

    return shell_escape_append_len(out, str, scan->quote, pad_unquoted, need);
}
