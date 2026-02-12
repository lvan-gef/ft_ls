#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_path.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"
#include "ft_str.h"

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

void printer(t_args *args, t_array *array) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);
    ASSERT_GT(array->len, 0);
    ASSERT_NOTNULL(array->data[0]);

    sort(array, args->reverse, args->time);
    if (args->list) {

    } else {
        print_row_(array);
    }
}

static void print_row_(t_array *array) {
    ASSERT_NOTNULL(array);
    ASSERT_GE(array->len, 1);
    ASSERT_NOTNULL(array->data[0]);

    size_t max_cols = max_(array);
    t_map map = {.cols = 1, .rows = max_cols, .max = max_cols};
    bool have_a_quoted = check_quoted_(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    uint64_t *col_widths = calc_cols_(arena, array, &map);
    if (!col_widths) {
        ArenaRelease(arena);
        return;
    }

    size_t *col_starts = ArenaPushNoZero(arena, map.cols * sizeof(*col_starts));
    if (!col_starts) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        ArenaRelease(arena);
        return;
    }

    col_starts[0] = 0;
    size_t index = 0;
    while (index < map.cols) {
        col_starts[index + 1] = col_starts[index] + col_widths[index] + 2;
        ++index;
    }

    size_t buf_size = TERM_SIZE + 16;
    t_str *buf = init_str(arena, buf_size);
    if (!buf) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc memory in arena\n");
        ArenaRelease(arena);
        return;
    }

    t_str *space = create_str(arena, " ");
    if (!space) {
        ArenaRelease(arena);
        return;
    }

    t_str *tab = create_str(arena, "\t");
    if (!tab) {
        ArenaRelease(arena);
        return;
    }

    t_str *new_line = create_str(arena, "\n");
    if (!new_line) {
        ArenaRelease(arena);
        return;
    }

    const size_t files_len = array->len;
    for (size_t row = 0; row < map.rows; ++row) {
        for (size_t col = 0; col < map.cols; ++col) {
            size_t idx = row + col * map.rows;
            if (idx >= files_len) {
                break;
            }

            const t_entry *entry = array->data[idx];
            bool is_last_col = (col == map.cols - 1) ||
                               (row + (col + 1) * map.rows >= files_len);

            if (!have_a_quoted && !entry->quoted->len) {
                cat_l_str(buf, space, buf_size - buf->len);
            } else if (have_a_quoted && entry->quoted->len) {
                cat_l_str(buf, entry->quoted, buf_size - buf->len);
            }

            cat_l_str(buf, entry->name, buf_size - buf->len);

            if (have_a_quoted && entry->quoted->len) {
                cat_l_str(buf, entry->quoted, buf_size - buf->len);
            }

            if (!is_last_col) {
                size_t target_pos = col_starts[col + 1];
                size_t gap = target_pos - buf->len;

                size_t test_pos = buf->len;
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
                    while (buf->len < target_pos) {
                        size_t next_tab = ((buf->len / 8) + 1) * 8;
                        if (next_tab <= target_pos) {
                            cat_l_str(buf, tab, buf_size - buf->len);
                        } else {
                            cat_l_str(buf, space, buf_size - buf->len);
                        }
                    }
                } else {
                    while (buf->len < target_pos) {
                        cat_l_str(buf, space, buf_size - buf->len);
                    }
                }
            }
        }

        cat_l_str(buf, new_line, buf_size - buf->len);
    }

    (void)write(STDOUT_FILENO, buf->str, buf->len);
    ArenaRelease(arena);
    return;
}

static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(array);
    ASSERT_GE(array->len, 1);
    ASSERT_NOTNULL(array->data[0]);
    ASSERT_NOTNULL(map);

    const uint64_t max_len = map->max;
    if (map->max > TERM_SIZE / 2) {
        map->max = TERM_SIZE / 2;
    }

    uint64_t *col_widths =
        ArenaPushNoZero(arena, map->max * sizeof(*col_widths));
    if (!col_widths) {
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

    ft_memset(col_widths, 0, num_cols * sizeof(*col_widths));
    // while (index < num_cols) {
    //     col_widths[index] = 0;
    //     ++index;
    // }

    uint64_t col = 0;
    while (col < num_cols) {
        uint64_t row = 0;
        while (row < num_rows) {
            index = row + col * num_rows;
            // if (index >= max_len) {
            if (index >= array->len) {
                break;
            }

            const t_entry *entry = array->data[index];
            ASSERT_NOTNULL(entry);
            ASSERT_GT(entry->name->len, 0);

            size_t len = entry->name->len;
            if (entry->quoted->len) {
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
    index = 0;
    while (index < num_cols) {
        total += col_widths[index];
        if (index < num_cols - 1) {
            ASSERT_(total + 2 > total, "total did overflow");
            total += 2;
        }

        ++index;
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
        if (entry->quoted->len) {
            return true;
        }

        ++index;
    }

    return false;
}
