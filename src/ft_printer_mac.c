#if defined(__APPLE__)

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_printer_mac.h"

bool print_cols_mac(t_path *path, size_t num_cols, size_t num_rows) {
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");

    size_t colwidth = (path->max_len + 8) & ~((size_t)7);
    const size_t files_len = path->files->len;

    for (size_t row = 0; row < num_rows; ++row) {
        for (size_t col = 0; col < num_cols; ++col) {
            size_t idx = row + col * num_rows;
            if (idx >= files_len) {
                break;
            }

            t_file *file = path->files->data[idx];
            if (write(STDOUT_FILENO, file->name->str, file->name->len) < 0) {
                return false;
            }

            bool is_last_in_row = (col == num_cols - 1) ||
                                  (row + (col + 1) * num_rows >= files_len);
            if (!is_last_in_row) {
                size_t cur_pos = file->name->len;
                while (cur_pos < colwidth) {
                    if (write(STDOUT_FILENO, "\t", 1) < 0) {
                        return false;
                    }
                    cur_pos = ((cur_pos / 8) + 1) * 8;
                }
            }
        }

        if (write(STDOUT_FILENO, "\n", 1) < 0) {
            return false;
        }
    }

    return true;
}

void calc_cols_mac(t_path *path, size_t *num_cols, size_t *num_rows) {
    ASSERT_(path, "path can not be NULL");
    ASSERT_(path->files, "path->files can not be NULL");
    ASSERT_(path->files->len, "path->files->len must be > 0");
    ASSERT_(num_cols, "num_cols can not be NULL");
    ASSERT_(num_rows, "num_rows can not be NULL");

    const size_t files_len = path->files->len;
    size_t colwidth = path->max_len;
    colwidth = (colwidth + 8) & ~((size_t)7);

    *num_cols = TERM_SIZE / colwidth;
    if (*num_cols < 1) {
        *num_cols = 1;
    }

    if (*num_cols > files_len) {
        *num_cols = files_len;
    }

    *num_rows = (files_len + *num_cols - 1) / *num_cols;
}
#endif // __APPLE__
