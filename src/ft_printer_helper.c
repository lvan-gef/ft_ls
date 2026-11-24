#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_printer.h"
#include "./ft_printer_helper.h"
#include "./ft_shell_escape.h"
#include "./ft_str.h"
#include "./ft_utils.h"

static const char *entry_color_(const t_entry *entry);

void put_entry_name(t_out *out, const t_entry *entry, const bool pad_unquoted,
                    const bool color, const bool is_stdout) {
    const t_str *name = entry->name ? entry->name : entry->path;

    if (!is_stdout) {
        out_put(out, name->str, name->len);
        return;
    }

    const char *start = color ? entry_color_(entry) : NULL;
    if (start) {
        out_str(out, start);
    }

    shell_escape_put(out, name, entry->name_scan.quote, pad_unquoted);

    if (start) {
        out_put(out, RESET, sizeof(RESET) - 1);
    }
}

void put_dir_header(t_out *out, const t_str *dir_header, const bool is_stdout) {
    if (!dir_header) {
        return;
    }

    if (!is_stdout) {
        out_put(out, dir_header->str, dir_header->len);
        out_put(out, ":\n", 2);
        return;
    }

    t_shell_scan scan;
    shell_scan_str(dir_header, &scan);
    if (scan.quote == '\0' &&
        ft_memchr(dir_header->str, ':', dir_header->len) != NULL) {
        scan.quote = '\'';
    }

    shell_escape_put(out, dir_header, scan.quote, false);
    out_put(out, ":\n", 2);
}

static const char *entry_color_(const t_entry *entry) {
    const mode_t mode = entry->st.st_mode;

    if (entry->stat_unavailable) {
        return NULL;
    }

    if (S_ISDIR(mode)) {
        return DIRECTORY;
    }

    if (S_ISLNK(mode)) {
        return SYMLINK;
    }

    if (S_ISSOCK(mode)) {
        return SOCKET;
    }

    if (S_ISFIFO(mode)) {
        return FIFO;
    }

    if (S_ISBLK(mode) || S_ISCHR(mode)) {
        return BLOCKCHAR;
    }

    if (S_ISREG(mode) && mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        return EXECUTABLE;
    }

    return NULL;
}
