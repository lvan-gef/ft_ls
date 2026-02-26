#include <stdint.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_path.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"
#include "../include/ft_print_list.h"

#include "../libft/include/ft_fprintf.h"
#include "../libft/include/libft.h"

#ifndef TABSIZE
#define TABSIZE UINT64_C(8)
#endif // ifndef TABSIZE //

#ifndef SPACE_GAP
#define SPACE_GAP UINT64_C(2)
#endif /* ifndef SPACE_GAP */

typedef struct {
    uint64_t rows;
    uint64_t cols;
    uint64_t max;
} t_map;

typedef struct {
    t_str *space;
    t_str *tab;
    t_str *new_line;
    t_str *dubble_colon;
} t_spacing;

static void init_print_row_(t_array *array, t_str *dir_path);
static void print_row_(t_array *array, t_str *buf, t_map *map,
                       t_spacing *spacing, bool quoted, uint64_t *col_starts);
static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map,
                            bool quoted);
static uint64_t calc_layout_width_(t_array *array, uint64_t num_cols,
                                   uint64_t *col_widths, bool quoted);
static bool check_quoted_(t_array *array);

void printer(t_args *args, t_array *array, t_str *dir_path, bool print_total) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);

    if (array->len) {
        sort(array, args->reverse, args->time);
    }

    if (args->list) {
        print_list(array, dir_path, print_total);
    } else {
        init_print_row_(array, dir_path);
    }
}

static void init_print_row_(t_array *array, t_str *dir_path) {
    ASSERT_NOTNULL(array);

    uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    if (max_cols < 1) {
        max_cols = 1;
    }

    t_map map = {.cols = 1,
                 .rows = array->len,
                 .max = array->len < max_cols ? array->len : max_cols};

    bool quoted = check_quoted_(array);
    const char *err_msg = NULL;

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        err_msg = "Failed to alloc arena for printer";
        goto done;
    }
    ArenaSetAutoAlign(arena, 8);

    uint64_t *col_widths = calc_cols_(arena, array, &map, quoted);
    if (!col_widths) {
        err_msg = "Failed to calc column leng";
        goto done;
    }

    uint64_t *col_starts =
        ArenaPushNoZero(arena, (map.cols + 1) * sizeof(*col_starts));
    if (!col_starts) {
        err_msg = "Failed to alloc from arena";
        goto done;
    }

    col_starts[0] = 0;
    for (uint64_t index = 0; index < map.cols; ++index) {
        col_starts[index + 1] =
            col_starts[index] + col_widths[index] + SPACE_GAP;
    }

    uint64_t buf_size = (TERM_SIZE * map.rows) + map.rows + 1;

    if (dir_path) {
        buf_size += dir_path->len + 3;
    }

    t_str *buf = init_str(arena, buf_size);
    if (!buf) {
        err_msg = "Failed to init t_str for buffer";
        goto done;
    }

    t_spacing spacing = {.space = create_str(arena, " "),
                         .tab = create_str(arena, "\t"),
                         .new_line = create_str(arena, "\n"),
                         .dubble_colon = create_str(arena, ":")};
    if (!spacing.space || !spacing.tab || !spacing.new_line ||
        !spacing.dubble_colon) {
        err_msg = "Failed to create spacing struct";
        goto done;
    }

    if (dir_path) {
        cat_l_str(buf, dir_path, buf->cap - buf->len);
        cat_l_str(buf, spacing.dubble_colon, buf->cap - buf->len);
        cat_l_str(buf, spacing.new_line, buf->cap - buf->len);
    }

    print_row_(array, buf, &map, &spacing, quoted, col_starts);
done:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }

    if (arena) {
        ArenaRelease(arena);
    }
}

static void print_row_(t_array *array, t_str *buf, t_map *map,
                       t_spacing *spacing, bool quoted, uint64_t *col_starts) {
    const uint64_t files_len = array->len;
    const uint64_t buf_size = buf->cap - 1;

    for (uint64_t row = 0; row < map->rows; ++row) {
        uint64_t curr_pos = 0;
        for (uint64_t col = 0; col < map->cols; ++col) {
            uint64_t idx = row + col * map->rows;
            if (idx >= files_len) {
                break;
            }

            const t_entry *entry = array->data[idx];
            bool is_last_col = (col == map->cols - 1) ||
                               (row + (col + 1) * map->rows >= files_len);

            if (entry->quoted->len) {
                cat_l_str(buf, entry->quoted, buf_size - buf->len);
                curr_pos += entry->quoted->len;
            } else if (quoted) {
                cat_l_str(buf, spacing->space, buf_size - buf->len);
                curr_pos += 1;
            }

            cat_l_str(buf, entry->name, buf_size - buf->len);
            curr_pos += entry->name->len;

            if (quoted && entry->quoted->len) {
                cat_l_str(buf, entry->quoted, buf_size - buf->len);
                curr_pos += entry->quoted->len;
            }

            if (is_last_col) {
                break;
            }

            uint64_t target_pos = col_starts[col + 1];
            uint64_t gap = target_pos - curr_pos;
            uint64_t test_pos = curr_pos;
            uint64_t num_tabs = 0;

            while (test_pos < target_pos) {
                uint64_t next_tab = ((test_pos / TABSIZE) + 1) * TABSIZE;
                if (next_tab > target_pos) {
                    break;
                }

                test_pos = next_tab;
                ++num_tabs;
            }

            uint64_t spaces_after_tabs = target_pos - test_pos;
            uint64_t chars_with_tabs = num_tabs + spaces_after_tabs;
            bool use_tabs = (num_tabs > 0) && (chars_with_tabs < gap);

            while (curr_pos < target_pos) {
                uint64_t next_tab = ((curr_pos / TABSIZE) + 1) * TABSIZE;
                if (use_tabs && next_tab <= target_pos) {
                    cat_l_str(buf, spacing->tab, buf_size - buf->len);
                    curr_pos = next_tab;
                } else {
                    cat_l_str(buf, spacing->space, buf_size - buf->len);
                    curr_pos++;
                }
            }
        }

        cat_l_str(buf, spacing->new_line, buf_size - buf->len);
    }

    (void)write(STDOUT_FILENO, buf->str, buf->len);
}

static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map,
                            bool quoted) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(array);
    ASSERT_NOTNULL(map);

    if (map->max > TERM_SIZE / 2) {
        map->max = TERM_SIZE / 2;
    }

    uint64_t *col_widths =
        ArenaPushNoZero(arena, (map->max + 1) * sizeof(*col_widths));
    if (!col_widths) {
        return NULL;
    }

    uint64_t width = 0;
    uint64_t try_cols = map->max;
    while (try_cols > 1) {
        width = calc_layout_width_(array, try_cols, col_widths, quoted);
        if (width < TERM_SIZE) {
            map->cols = try_cols;
            map->rows = (array->len + map->cols - 1) / map->cols;
            return col_widths;
        }
        --try_cols;
    }

    (void)calc_layout_width_(array, 1, col_widths, quoted);
    return col_widths;
}

static uint64_t calc_layout_width_(t_array *array, uint64_t num_cols,
                                   uint64_t *col_widths, bool quoted) {
    uint64_t num_rows = (array->len + num_cols - 1) / num_cols;
    uint64_t index = 0;

    ft_memset(col_widths, 0, num_cols * sizeof(*col_widths));

    for (uint64_t col = 0; col < num_cols; ++col) {
        for (uint64_t row = 0; row < num_rows; ++row) {
            index = row + col * num_rows;
            if (index >= array->len) {
                break;
            }

            const t_entry *entry = array->data[index];
            ASSERT_NOTNULL(entry);
            ASSERT_GT(entry->name->len, 0);

            uint64_t len = entry->name->len;
            if (entry->quoted->len) {
                len += entry->quoted->len * 2;
            } else if (quoted) {
                len += 1;
            }

            if (len > col_widths[col]) {
                col_widths[col] = len;
            }
        }
    }

    uint64_t total = 0;
    for (index = 0; index < num_cols; ++index) {
        total += col_widths[index];
        if (index < num_cols - 1) {
            ASSERT_(total + SPACE_GAP > total, "total did overflow");
            total += SPACE_GAP;
        }
    }

    return total;
}

static bool check_quoted_(t_array *array) {
    for (uint64_t index = 0; index < array->len; ++index) {
        t_entry *entry = array->data[index];
        if (entry->quoted->len) {
            return true;
        }
    }

    return false;
}
