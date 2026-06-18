#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ft_walk_internal.h"

#include "../libft/include/ft_fprintf.h"

#include "ft_printer_helper.h"
#include "ft_shell_escape.h"

bool walk_path_print_error(t_str *out, const t_str *path, const int e,
                           const char *prefix, bool *output_failed) {
    if (!flush_str(out)) {
        if (output_failed) {
            *output_failed = true;
        }
        return false;
    }

    t_shell_scan scan;
    shell_scan_str(path, &scan);
    if (scan.quote != '\0') {
        t_str *escaped = shell_escape_str(path, scan.quote);
        if (escaped) {
            ft_fprintf(STDERR_FILENO, "ft_ls: %s %s: %s\n", prefix,
                       escaped->str, strerror(e));
            str_free(escaped);
            return true;
        }
    }

    ft_fprintf(STDERR_FILENO, "ft_ls: %s '%s': %s\n", prefix,
               path->str + path->pos, strerror(e));
    return true;
}

mode_t walk_dtype_to_mode(const unsigned char dtype) {
    switch (dtype) {
        case DT_BLK: return S_IFBLK;
        case DT_CHR: return S_IFCHR;
        case DT_DIR: return S_IFDIR;
        case DT_FIFO: return S_IFIFO;
        case DT_LNK: return S_IFLNK;
        case DT_REG: return S_IFREG;
        case DT_SOCK: return S_IFSOCK;
        default: return 0;
    }
}

bool walk_dtype_needs_lstat(const bool list, const bool time,
                            const unsigned char dtype) {
    if (list || time) {
        return true;
    }

    return walk_dtype_to_mode(dtype) == 0;
}
