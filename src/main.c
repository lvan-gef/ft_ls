#include "../include/ft_assert.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"
#include "../include/ft_walk.h"
#include "../include/ft_ls.h"
#include "../include/ft_arena.h"

static void clean_program(Arena *arena);

int main(int argc, char **argv) {
    t_args args = {0};
    Arena *arena = ArenaAlloc((U64)4096 * 2);
    if (!arena) {
        return 1;
    }

    if (argc > 1) {
        if (!parse_args(arena, argc, argv, &args)) {
            clean_program(arena);
            return 3;
        }
    } else {
        if (!default_arg(&args)) {
            clean_program(arena);
            return 4;
        }
    }

    if (!walk(&args)) {
        clean_program(arena);
        return 5;
    }

    print_ls(&args);

    clean_program(arena);
    return 0;
}

static void clean_program(Arena *arena) {
    ASSERT_(arena, "arena can not be NULL");

    ArenaRelease(arena);
}
