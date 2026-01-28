#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef uint64_t U64;

typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    U64 pos;
    U64 cap;
    ArenaBlock *next;
    ArenaBlock *prev;
};

typedef struct {
    U64 align;
    U64 block_size;
    ArenaBlock *first;
    ArenaBlock *current;
} Arena;

/**
 * @brief Creates a new arena with the specified capacity.
 * @param cap (U64) Capacity in bytes.
 * @return (Arena*) Pointer to the arena, or NULL if allocation fails.
 */
Arena *ArenaAlloc(U64 cap);

/**
 * @brief Frees the arena and all its memory.
 * @param arena (Arena*) Pointer to the arena to release.
 */
void ArenaRelease(Arena *arena);

/**
 * @brief Sets automatic alignment for all subsequent allocations.
 * @param arena (Arena*) Pointer to the arena.
 * @param align (U64) Alignment in bytes. Set to 0 to disable.
 */
void ArenaSetAutoAlign(Arena *arena, U64 align);

/**
 * @brief Returns the current position in the current block.
 * @param arena (Arena*) Pointer to the arena.
 * @return (U64) Current byte offset within the current block.
 * @note Only valid for use with ArenaPopTo within the same block.
 */
U64 ArenaPos(Arena *arena);

/**
 * @brief Allocates memory from the arena without zeroing it.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (U64) Number of bytes to allocate.
 * @return (void*) Pointer to the allocated memory. Aborts if out of capacity.
 */
void *ArenaPushNoZero(Arena *arena, U64 size);

/**
 * @brief Allocates zeroed memory from the arena.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (U64) Number of bytes to allocate.
 * @return (void*) Pointer to the zeroed memory. Aborts if out of capacity.
 */
void *ArenaPush(Arena *arena, U64 size);

/**
 * @brief Aligns the arena position and allocates a block of that alignment
 * size.
 * @param arena (Arena*) Pointer to the arena.
 * @param alignment (U64) Alignment and allocation size in bytes.
 * @return (void*) Pointer to the aligned, zeroed memory. Aborts if out of
 * capacity.
 */
void *ArenaPushAligner(Arena *arena, U64 alignment);

/**
 * @brief Resets the arena position to a previously saved position.
 * @param arena (Arena*) Pointer to the arena.
 * @param pos (U64) Position to reset to. Ignored if greater than current
 * position.
 * @note Only works within the current block. Use ArenaClear to reset across all
 * blocks.
 */
void ArenaPopTo(Arena *arena, U64 pos);

/**
 * @brief Pops a number of bytes from the end of the arena.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (U64) Number of bytes to pop. Resets to 0 if size exceeds current
 * position.
 * @note Only works within the current block. Use ArenaClear to reset across all
 * blocks.
 */
void ArenaPop(Arena *arena, U64 size);

/**
 * @brief Resets the arena position to 0, making all memory available again.
 * @param arena (Arena*) Pointer to the arena.
 */
void ArenaClear(Arena *arena);

#endif // !ARENA_H
