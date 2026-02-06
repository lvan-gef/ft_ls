#define FT_ASSERT_IMPLEMENTATION
#include "../include/ft_assert.h"
#include "../include/ft_arena.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Arena *arena = ArenaAlloc(0);
    ASSERT_NOTNULL(arena);
    if (!arena) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
