#include <stdint.h>
#include <unistd.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
#include "../include/ft_parse.h"
#include "../include/ft_print_list.h"
#include "../include/ft_printer.h"
#include "../include/ft_sort.h"
#include "../include/ft_str.h"

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

static void init_print_row_(Arena *arena, t_array *array, t_entry *dir_entry,
                            bool force_quote_padding);
static void print_row_(t_array *array, t_str *buf, const t_map *map,
                       const t_spacing *spacing, bool quoted,
                       const uint64_t *col_starts);
static uint64_t *calc_cols_(Arena *arena, t_array *array, t_map *map,
                            bool quoted);
static uint64_t calc_width_(Arena *arena, t_array *array, uint64_t num_cols,
                            uint64_t *col_widths, bool quoted);
static bool check_quoted_(t_array *array);

void printer(const t_args *args, t_array *array, t_entry *dir_entry,
             bool print_total, uint64_t min_len_links, uint64_t min_len_sizes,
             bool force_quote_padding) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        ft_fprintf(STDERR_FILENO, "Failed to alloc arena for printer");
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    if (array->len) {
        sort(arena, array, args->reverse, args->time);
    }

    if (args->list) {
        print_list(array, dir_entry, print_total, min_len_links, min_len_sizes,
                   force_quote_padding);
    } else {
        init_print_row_(arena, array, dir_entry, force_quote_padding);
    }

    ArenaRelease(arena);
}

static void init_print_row_(Arena *arena, t_array *array, t_entry *dir_entry,
                            bool force_quote_padding) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(array);

    uint64_t max_cols = (TERM_SIZE + SPACE_GAP) / (1 + SPACE_GAP);
    if (max_cols < 1) {
        max_cols = 1;
    }

    t_map map = {.cols = 1,
                 .rows = array->len,
                 .max = array->len < max_cols ? array->len : max_cols};

    bool quoted = force_quote_padding || check_quoted_(array);
    const char *err_msg = NULL;

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

    uint64_t row_width =
        calc_width_(arena, array, map.cols, col_widths, quoted);
    uint64_t buf_size = (row_width * map.rows) + map.rows + 1;

    if (dir_entry) {
        dir_entry = escape_entry(arena, dir_entry);
        if (!dir_entry) {
            err_msg = "Failed to escape dir";
            goto done;
        }

        if (dir_entry->quoted && dir_entry->quoted->len) {
            buf_size += 2;
        }
        buf_size += dir_entry->name->len + 3; // 1 for space, 1 for :, 1 for \n
    }

    t_str *buf = init_str(arena, buf_size);
    if (!buf) {
        err_msg = "Failed to init t_str for buffer";
        goto done;
    }

    char space_buf[] = " ";
    char tab_buf[] = "\t";
    char new_line_buf[] = "\n";
    char dubble_colon_buf[] = ":";

    t_str space = {.str = space_buf, .cap = 2, .len = 1, .pos = 0};
    t_str tab = {.str = tab_buf, .cap = 2, .len = 1, .pos = 0};
    t_str new_line = {.str = new_line_buf, .cap = 2, .len = 1, .pos = 0};
    t_str dubble_colon = {
        .str = dubble_colon_buf, .cap = 2, .len = 1, .pos = 0};

    t_spacing spacing = {.space = &space,
                         .tab = &tab,
                         .new_line = &new_line,
                         .dubble_colon = &dubble_colon};

    if (dir_entry) {
        if (dir_entry->quoted && dir_entry->quoted->len) {
            cat_str(buf, dir_entry->quoted);
            cat_str(buf, dir_entry->name);
            cat_str(buf, dir_entry->quoted);
        } else {
            cat_str(buf, dir_entry->name);
        }

        cat_str(buf, spacing.dubble_colon);
        cat_str(buf, spacing.new_line);
    }

    print_row_(array, buf, &map, &spacing, quoted, col_starts);
done:
    if (err_msg) {
        ft_fprintf(STDERR_FILENO, "%s\n", err_msg);
    }
}

static void print_row_(t_array *array, t_str *buf, const t_map *map,
                       const t_spacing *spacing, bool quoted,
                       const uint64_t *col_starts) {
    const uint64_t files_len = array->len;

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
                cat_str(buf, entry->quoted);
                curr_pos += entry->quoted->len;
            } else if (quoted) {
                cat_str(buf, spacing->space);
                curr_pos += 1;
            }

            cat_str(buf, entry->name);
            curr_pos += entry->name->len;

            if (quoted && entry->quoted->len) {
                cat_str(buf, entry->quoted);
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
                    cat_str(buf, spacing->tab);
                    curr_pos = next_tab;
                } else {
                    cat_str(buf, spacing->space);
                    curr_pos++;
                }
            }
        }

        cat_str(buf, spacing->new_line);
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

    uint64_t try_cols = map->max;
    while (try_cols > 1) {
        uint64_t width =
            calc_width_(arena, array, try_cols, col_widths, quoted);
        if (width < TERM_SIZE) {
            map->cols = try_cols;
            map->rows = (array->len + map->cols - 1) / map->cols;
            return col_widths;
        }
        --try_cols;
    }

    (void)calc_width_(arena, array, 1, col_widths, quoted);
    return col_widths;
}

static uint64_t calc_width_(Arena *arena, t_array *array, uint64_t num_cols,
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

            t_entry *entry = array->data[index];
            ASSERT_NOTNULL(entry);
            ASSERT_GT(entry->name->len, 0);

            entry = escape_entry(arena, entry);
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
        const t_entry *entry = array->data[index];
        if (entry->quoted->len) {
            return true;
        }
    }

    return false;
}
