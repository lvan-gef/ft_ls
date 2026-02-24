#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_print_list.h"
#include "ft_arena.h"
#include "ft_fprintf.h"
#include "ft_path.h"
#include "ft_str.h"


uint64_t get_buf_size_(t_array *array);

void print_list(t_array *array) {
    ASSERT_NOTNULL(array);

    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return;
    }
    ArenaSetAutoAlign(arena, 8);

    uint64_t buf_size = get_buf_size_(array);
    t_str *buffer = init_str(arena, buf_size);
    if (!buffer) {
        goto done;
    }

    // perms + links + username + group + size + datetime + file + symlink

done:
    ArenaRelease(arena);
}

uint64_t get_buf_size_(t_array *array) {
    ASSERT_NOTNULL(array);
    return 0;
}
