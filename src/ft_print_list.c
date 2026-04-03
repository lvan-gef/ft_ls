#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
#include "../include/ft_print_list.h"
#include "../include/ft_str.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

typedef struct {
    uint64_t total;
    uint64_t max_len_links;
    uint64_t max_len_sizes;
    uint64_t max_len_perm;
    bool have_quote;
} t_sizes;

typedef struct {
    uint64_t len;
    char data[4096];
} t_output;

static void get_sizes_(t_array *array, t_sizes *sizes);
static bool printer_(t_output *out, t_array *array, const t_sizes *sizes);
static bool left_pad_(t_output *out, uint64_t src_len, uint64_t max_size);
static bool have_quotes_(t_array *array);
static bool flush_output_(t_output *out);
static bool put_mem_(t_output *out, const char *src, uint64_t len);
static bool put_char_(t_output *out, char c);
static bool put_fill_(t_output *out, char c, uint64_t count);
static bool put_str_(t_output *out, const t_str *str);
static bool put_uint_(t_output *out, uint64_t value);
static bool put_display_name_(t_output *out, const t_entry *entry,
                              bool pad_unquoted);
static bool write_mem_(void *ctx, const char *src, uint64_t len);

void print_list(t_array *array, t_entry *dir_entry, bool print_total,
                uint64_t min_len_links, uint64_t min_len_sizes,
                bool force_quote_padding) {
    ASSERT_NOTNULL(array);

    t_sizes sizes = {.have_quote = force_quote_padding || have_quotes_(array),
                     .max_len_links = min_len_links,
                     .max_len_sizes = min_len_sizes};
    t_output out = {0};
    const char *err_msg = NULL;

    get_sizes_(array, &sizes);

    if (dir_entry) {
        if (!put_display_name_(&out, dir_entry, false) ||
            !put_mem_(&out, ":\n", 2)) {
            err_msg = "Failed to write dir header";
            goto done;
        }
    }

    if (print_total) {
        if (!put_mem_(&out, "total ", 6) ||
            !put_uint_(&out, (sizes.total + 1) / 2) || !put_char_(&out, '\n')) {
            err_msg = "Failed to write total";
            goto done;
        }
    }

    if (!printer_(&out, array, &sizes) || !flush_output_(&out)) {
        err_msg = "Failed to write output";
    }

done:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }
}

static bool printer_(t_output *out, t_array *array, const t_sizes *sizes) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(sizes);

    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];

        if (!put_str_(out, entry->info->perm) ||
            !left_pad_(out, entry->info->perm->len, sizes->max_len_perm) ||
            !put_char_(out, ' ') ||
            !left_pad_(out, entry->info->links->len, sizes->max_len_links) ||
            !put_str_(out, entry->info->links) || !put_char_(out, ' ') ||
            !put_str_(out, entry->info->username) || !put_char_(out, ' ') ||
            !put_str_(out, entry->info->groupname) || !put_char_(out, ' ') ||
            !left_pad_(out, entry->info->size->len, sizes->max_len_sizes) ||
            !put_str_(out, entry->info->size) || !put_char_(out, ' ') ||
            !put_str_(out, entry->info->dt) || !put_char_(out, ' ') ||
            !put_display_name_(out, entry, sizes->have_quote)) {
            return false;
        }

        if (entry->info->symlink) {
            if (!put_mem_(out, " -> ", 4) ||
                !put_str_(out, entry->info->symlink)) {
                return false;
            }
        }

        if (!put_char_(out, '\n')) {
            return false;
        }
    }

    return true;
}

static void get_sizes_(t_array *array, t_sizes *sizes) {
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(sizes);

    for (uint64_t i = 0; i < array->len; ++i) {
        const t_entry *e = array->data[i];

        if (e->info->links->len > sizes->max_len_links) {
            sizes->max_len_links = e->info->links->len;
        }

        if (e->info->size->len > sizes->max_len_sizes) {
            sizes->max_len_sizes = e->info->size->len;
        }

        if (e->info->perm->len > sizes->max_len_perm) {
            sizes->max_len_perm = e->info->perm->len;
        }

        sizes->total += e->info->blocks;
    }
}

static bool left_pad_(t_output *out, uint64_t src_len, uint64_t max_size) {
    ASSERT_NOTNULL(out);
    ASSERT_LE(src_len, max_size);

    return put_fill_(out, ' ', max_size - src_len);
}

static bool have_quotes_(t_array *array) {
    ASSERT_NOTNULL(array);

    for (uint64_t index = 0; index < array->len; ++index) {
        const t_entry *entry = array->data[index];
        if (entry->quote != '\0') {
            return true;
        }
    }

    return false;
}

static bool flush_output_(t_output *out) {
    ASSERT_NOTNULL(out);

    uint64_t written = 0;
    while (written < out->len) {
        const ssize_t chunk = write(STDOUT_FILENO, out->data + written,
                                    (size_t)(out->len - written));
        if (chunk < 0) {
            return false;
        }
        written += (uint64_t)chunk;
    }

    out->len = 0;
    return true;
}

static bool put_mem_(t_output *out, const char *src, uint64_t len) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(src);

    while (len) {
        if (out->len == sizeof(out->data) && !flush_output_(out)) {
            return false;
        }

        const uint64_t avail = sizeof(out->data) - out->len;
        const uint64_t to_copy = len < avail ? len : avail;
        ft_memcpy(out->data + out->len, src, (size_t)to_copy);
        out->len += to_copy;
        src += to_copy;
        len -= to_copy;
    }

    return true;
}

static bool put_char_(t_output *out, char c) {
    return put_mem_(out, &c, 1);
}

static bool put_fill_(t_output *out, char c, uint64_t count) {
    ASSERT_NOTNULL(out);

    while (count) {
        if (out->len == sizeof(out->data) && !flush_output_(out)) {
            return false;
        }

        const uint64_t avail = sizeof(out->data) - out->len;
        const uint64_t to_fill = count < avail ? count : avail;
        ft_memset(out->data + out->len, c, (size_t)to_fill);
        out->len += to_fill;
        count -= to_fill;
    }

    return true;
}

static bool put_str_(t_output *out, const t_str *str) {
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    return put_mem_(out, str->str, str->len);
}

static bool put_uint_(t_output *out, uint64_t value) {
    char digits[32];
    size_t index = sizeof(digits);

    do {
        digits[--index] = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0);

    return put_mem_(out, digits + index, (uint64_t)(sizeof(digits) - index));
}

static bool write_mem_(void *ctx, const char *src, uint64_t len) {
    return put_mem_(ctx, src, len);
}

static bool put_display_name_(t_output *out, const t_entry *entry,
                              bool pad_unquoted) {
    ASSERT_NOTNULL(out);
    ASSERT_NOTNULL(entry);
    ASSERT_NOTNULL(entry->name);

    return write_shell_escaped(out, write_mem_, entry->name, entry->quote,
                               pad_unquoted);
}
