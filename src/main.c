#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_ls.h"
#include "../include/ft_parser.h"
#include "../include/ft_print.h"

static void clean_program(t_args *args);

int main(int argc, char **argv) {
    t_args args = {0};
    Arena *arena = ArenaAlloc((U64)ARENA_SIZE * 2);
    if (!arena) {
        return 1;
    }
    ArenaSetAutoAlign(arena, 8);

    args.paths = init_array(arena, DEFAULT_SIZE, ARRAY_PATHS);
    if (!args.paths) {
        clean_program(&args);
        return 1;
    }

    if (argc > 1) {
        if (!parse_args(argc, argv, &args)) {
            clean_program(&args);
            return 1;
        }
    } else {
        if (!default_arg(&args)) {
            clean_program(&args);
            return 1;
        }
    }

    printer(&args);
    // print_ls(&args);

    clean_program(&args);
    return 0;
}

static void clean_program(t_args *args) {
    ASSERT_(args, "args can not be NULL");

    ArenaRelease(args->paths->arena);
}
