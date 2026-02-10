#define C_ASSERT_IMPLEMENTATION

#include <stdlib.h>

#include "../include/ft_arena.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_walk.h"

void clean_program_(Arena *arena);

int main(int argc, char *argv[]) {
    t_args args = {0};
    Arena *arena = ArenaAlloc(ARENA_SIZE);
    if (!arena) {
        return EXIT_FAILURE;
    }
    ArenaSetAutoAlign(arena, 8);

    t_array *inputs = parse_args(arena, argc, argv, &args);
    if (!inputs) {
        clean_program_(arena);
        return EXIT_FAILURE;
    }

    int exit_code = 0;
    process(&args, inputs, &exit_code);

    clean_program_(arena);
    return exit_code;
}

void clean_program_(Arena *arena) {
    ASSERT_NOTNULL(arena);

    ArenaRelease(arena);
}
