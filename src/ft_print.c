#include <unistd.h>

#include "../include/ft_print.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_sort.h"
#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"


static void printer_(const t_args *args, t_array *files, size_t len);
static void print_single_row(t_array *files);
// static void set_padding_(char *padding, size_t len, size_t file_len,
//                          size_t rows);
static size_t get_rows_(size_t file_count, size_t len);
// static char *need_quote_(const char *str);

void print_ls(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    t_array *paths = args->paths;
    if (paths->len == 0) {
        return;
    } else if (paths->len > 1) {
        size_t index = 0;
        while (index < paths->len) {
            t_path *path = paths->data[index];
            ASSERT_(path->max_len, "path->max_len must be more then 0");
            printer_(args, path->files, path->max_len);
            ++index;
        }
    } else {
        t_path *path = paths->data[0];
        printer_(args, path->files, path->max_len);
    }
}

// TODO: when multi row then print up -> down left -> rigth
static void printer_(const t_args *args, t_array *files, size_t max_len) {
    ASSERT_(args, "args can not be NULL");
    ASSERT_(files, "files can not be NULL");
    ASSERT_(max_len, "len must be more then 0");

    // const size_t files_per_row = files->len / rows;

    if (args->time) {
        sort_time(files, args->reverse);
    } else {
        sort_alpha(files);
    }
    size_t rows = get_rows_(files->len, max_len);
    if (rows == 1) {
        print_single_row(files);
    } else {
        exit(88);
    }

    // size_t file_printed = 0;
    // size_t index = 0;
    // while (index < files->len) {
    //     t_file *file = files->data[index];
    //
    //     if (file_printed >= files_per_row) {
    //         ft_fprintf(STDOUT_FILENO, "\n");
    //         file_printed = 0;
    //     }
    //
    //     const char *c = need_quote_(file->filename);
    //     const char *next_is_quoted = NULL;
    //     if (index + 1 < files->len) {
    //         const t_file *next = files->data[index + 1];
    //         next_is_quoted = need_quote_(next->filename);
    //     }
    //
    //     size_t new_len = file->len;
    //     char padding[max_len + 1];
    //     if (c) {
    //         ASSERT_(new_len - 2 < file->len, "new_len did underflow");
    //         new_len -= 2;
    //     }
    //     set_padding_(padding, max_len, new_len, rows);
    //
    //     if (!c) {
    //         if (next_is_quoted) {
    //             ft_fprintf(STDOUT_FILENO, "%s%s", file->filename, padding);
    //         } else {
    //             ft_fprintf(STDOUT_FILENO, "%s %s", file->filename, padding);
    //         }
    //     } else {
    //         char *quote = "'";
    //         if (*c == '\'') {
    //             quote = "\"";
    //         }
    //
    //         if (next_is_quoted) {
    //             ft_fprintf(STDOUT_FILENO, "%s%s%s%s", quote, file->filename, quote, padding);
    //         } else {
    //             ft_fprintf(STDOUT_FILENO, "%s%s%s %s", quote, file->filename, quote, padding);
    //         }
    //     }
    //
    //     ++file_printed;
    //     ++index;
    // }
    //
    // ft_fprintf(STDOUT_FILENO, "\n");
}

static void print_single_row(t_array *files) {
    size_t total_len = 0;
    size_t index = 0;

    ft_fprintf(STDOUT_FILENO, "%d\n", files->len);
    while (index < files->len - 1) {
        t_file *f = files->data[index];
        ft_fprintf(STDERR_FILENO, "%s\n", f->filename);
        total_len += ft_strlen(f->filename) + 2;
        ++index;
    }
    total_len += ft_strlen(files->data[index]);
    total_len += 2;

    char buf[total_len];
    index = 0;
    t_file *f = files->data[index];
    ft_strlcpy(buf, f->filename, total_len);
    ft_strlcat(buf, "  ", total_len);
    ++index;
    while (index < files->len) {
        f = files->data[index];
        ft_strlcat(buf, f->filename, total_len);
        ft_strlcat(buf, "  ", total_len);
        ++index;
    }
    ft_strlcat(buf, "\n", total_len);

    // ft_fprintf(STDOUT_FILENO, "%s\n", buf);
    write(STDOUT_FILENO, buf, total_len);
}

static size_t get_rows_(size_t file_count, size_t max_len) {
    ASSERT_(file_count, "file_count should be more then 0");
    ASSERT_(max_len, "len should be more then 0");

    const size_t new_len = max_len;
    size_t tmp_count = file_count / 2;

    size_t tmp_width = new_len * tmp_count;  // make a assert for it??
    size_t rows = 1;

    // make asserts in the while loop??
    while (tmp_width > TERM_SIZE) {
        ++rows;
        tmp_count = tmp_count / 2;
        tmp_width = new_len * tmp_count;
    }

    return rows;
}

// static void set_padding_(char *padding, size_t max_len, size_t file_len,
//                          size_t rows) {
//     ASSERT_(file_len, "file_len should be more then 0");
//     ASSERT_(max_len, "len should be more then 0");
//     ASSERT_(max_len >= file_len, "len should be >= then file_len");
//     ASSERT_(rows, "rows should be more then 0");
//
//     // size_t index = 0;
//     // size_t differ = 2;
//     // if (rows > 1) {
//     //     differ = len - file_len;
//     // }
//
//     size_t index = 0;
//     size_t differ = max_len - file_len;
//
//     while (index < differ) {
//         padding[index] = ' ';
//         ++index;
//     }
//
//     padding[index] = '\0';
// }
//
// static char *need_quote_(const char *str) {
//     size_t index = 0;
//
//     while (str[index]) {
//         char *c = ft_strchr(" '", str[index]);
//         if (c) {
//             return c;
//         }
//
//         ++index;
//     }
//
//     return NULL;
// }
