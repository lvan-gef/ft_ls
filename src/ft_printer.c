#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_path.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"
#include "ft_arena.h"
#include "ft_fprintf.h"
#include "libft.h"

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

static void print_row_(t_array *array);
static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map);
static uint64_t calc_layout_width_(t_array *array, uint64_t num_cols,
                                   uint64_t *col_widths, uint64_t max_len);
static uint64_t max_(t_array *array);
static bool check_quoted_(t_array *array);

void printer(t_args *args, t_array *files) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(files);
    ASSERT_GT(files->len, 0);
    ASSERT_NOTNULL(files->data[0]);

    sort(files, args->reverse, args->time);
    if (args->list) {

    } else {
        print_row_(files);
    }
}

static void print_row_(t_array *array) {
    size_t max_cols = max_(array);
    t_map map = {.cols = 1, .rows = max_cols, .max = max_cols};
    bool have_a_quoted = check_quoted_(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }

    uint64_t *col_widths = calc_cols_(arena, array, &map);
    if (!col_widths) {
        ArenaRelease(arena);
        return;
    }

    size_t *col_starts = ArenaPush(arena, (map.cols + 1) * sizeof(*col_starts));
    if (!col_starts) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        ArenaRelease(arena);
        return;
    }

    col_starts[0] = 0;
    for (size_t c = 0; c < map.cols; ++c) {
        col_starts[c + 1] = col_starts[c] + col_widths[c] + 2;
    }

    size_t buf_size = TERM_SIZE + 16;
    char *buf = ArenaPush(arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        ArenaRelease(arena);
        return;
    }

    const size_t files_len = array->len;
    for (size_t row = 0; row < map.rows; ++row) {
        size_t buf_len = 0;
        size_t cur_pos = 0;

        for (size_t col = 0; col < map.cols; ++col) {
            size_t idx = row + col * map.rows;
            if (idx >= files_len) {
                break;
            }

            const t_entry *entry = array->data[idx];
            bool is_last_col = (col == map.cols - 1) ||
                               (row + (col + 1) * map.rows >= files_len);

            // alleen als er iemand quoted heeft
            if (!have_a_quoted && !entry->quoted->len) {
                buf[buf_len] = ' ';
                ++buf_len;
                ++cur_pos;
            } else if (have_a_quoted && entry->quoted) {
                buf_len += ft_strlcpy(buf + buf_len, entry->quoted->str,
                                      buf_size - buf_len);
                ++cur_pos;
            }

            buf_len +=
                ft_strlcpy(buf + buf_len, entry->name->str, buf_size - buf_len);
            cur_pos += entry->name->len;

            if (have_a_quoted && entry->quoted->len) {
                buf_len += ft_strlcpy(buf + buf_len, entry->quoted->str,
                                      buf_size - buf_len);
                ++cur_pos;
            }

            if (!is_last_col) {
                size_t target_pos = col_starts[col + 1];
                size_t gap = target_pos - cur_pos;

                size_t test_pos = cur_pos;
                size_t num_tabs = 0;
                while (test_pos < target_pos) {
                    size_t next_tab = ((test_pos / 8) + 1) * 8;
                    if (next_tab > target_pos) {
                        break;
                    }

                    test_pos = next_tab;
                    ++num_tabs;
                }

                size_t spaces_after_tabs = target_pos - test_pos;
                size_t chars_with_tabs = num_tabs + spaces_after_tabs;
                if (num_tabs > 0 && chars_with_tabs < gap) {
                    while (cur_pos < target_pos) {
                        size_t next_tab = ((cur_pos / 8) + 1) * 8;
                        if (next_tab <= target_pos) {
                            buf[buf_len] = '\t';
                            ++buf_len;
                            cur_pos = next_tab;
                        } else {
                            buf[buf_len] = ' ';
                            ++buf_len;
                            ++cur_pos;
                        }
                    }
                } else {
                    while (cur_pos < target_pos) {
                        buf[buf_len] = ' ';
                        ++buf_len;
                        ++cur_pos;
                    }
                }
            }
        }

        buf[buf_len] = '\n';
        ++buf_len;
        if (write(STDOUT_FILENO, buf, buf_len) < 0) {
            break;
        }
    }

    ArenaRelease(arena);
    return;
}

static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map) {
    const uint64_t max_len = map->max;
    if (map->max > TERM_SIZE / 2) {
        map->max = TERM_SIZE / 2;
    }

    uint64_t *col_widths =
        ArenaPushNoZero(arena, map->max * sizeof(*col_widths));
    if (!*col_widths) {
        return NULL;
    }

    uint64_t width = 0;
    uint64_t try_cols = map->max;
    while (try_cols > 1) {
        width = calc_layout_width_(array, try_cols, col_widths, max_len);
        if (width < TERM_SIZE) {
            map->cols = try_cols;
            map->rows = (max_len + map->cols - 1) / map->cols;
            break;
        }
        --try_cols;
    }

    (void)calc_layout_width_(array, map->max, col_widths, max_len);

    return col_widths;
}

static uint64_t calc_layout_width_(t_array *array, uint64_t num_cols,
                                   uint64_t *col_widths, uint64_t max_len) {
    uint64_t num_rows = (max_len + num_cols - 1) / num_cols;
    uint64_t index = 0;

    while (index < num_cols) {
        col_widths[index] = 0;
        ++index;
    }

    uint64_t col = 0;
    while (col < num_cols) {
        uint64_t row = 0;
        while (row < num_rows) {
            index = row + col * num_rows;
            if (index >= max_len) {
                break;
            }

            const t_entry *entry = array->data[index];
            ASSERT_NOTNULL(entry);
            ASSERT_GT(entry->name->len, 0);

            size_t len = entry->name->len;
            if (entry->quoted) {
                ASSERT_(len + 1 > len, "len did overflow");
                len += 1;
            }

            if (len > col_widths[col]) {
                col_widths[col] = len;
            }
            ++row;
        }
        ++col;
    }

    size_t total = 0;
    for (size_t c = 0; c < num_cols; ++c) {
        total += col_widths[c];
        if (c < num_cols - 1) {
            ASSERT_(total + 2 > total, "total did overflow");
            total += 2;
        }
    }

    return total;
}

static uint64_t max_(t_array *array) {
    uint64_t len = 0;
    uint64_t index = 0;

    while (index < array->len) {
        t_entry *entry = array->data[index];
        if (entry->name->len > len) {
            len = entry->name->len;
        }

        ++index;
    }

    return len;
}

static bool check_quoted_(t_array *array) {
    uint64_t index = 0;

    while (index < array->len) {
        t_entry *entry = array->data[index];
        if (entry->quoted) {
            return true;
        }

        ++index;
    }

    return false;
}
