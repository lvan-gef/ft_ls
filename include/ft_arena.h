#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    uint64_t pos;
    uint64_t cap;
    ArenaBlock *next;
    ArenaBlock *prev;
};

typedef struct {
    uint64_t align;
    uint64_t block_size;
    ArenaBlock *first;
    ArenaBlock *current;
} Arena;

typedef struct {
    ArenaBlock *block;
    uint64_t pos;
} ArenaMark;

#ifndef ARENA_SIZE
#define ARENA_SIZE UINT64_C(4096)
#endif // !ARENA_SIZE

/**
 * @brief Creates a new arena with the specified capacity.
 * @param cap (uint64_t) Capacity in bytes.
 * @return (Arena*) Pointer to the arena, or NULL if allocation fails.
 */
Arena *ArenaAlloc(uint64_t cap);

/**
 * @brief Frees the arena and all its memory.
 * @param arena (Arena*) Pointer to the arena to release.
 */
void ArenaRelease(Arena *arena);

/**
 * @brief Sets automatic alignment for all subsequent allocations.
 * @param arena (Arena*) Pointer to the arena.
 * @param align (uint64_t) Alignment in bytes. Set to 0 to disable.
 */
void ArenaSetAutoAlign(Arena *arena, uint64_t align);

/**
 * @brief Returns the current position in the current block.
 * @param arena (Arena*) Pointer to the arena.
 * @return (uint64_t) Current byte offset within the current block.
 * @note Only valid for use with ArenaPopTo within the same block.
 */
uint64_t ArenaPos(Arena *arena);

/**
 * @brief Captures the current arena state for later rollback.
 * @param arena (Arena*) Pointer to the arena.
 * @return (ArenaMark) Block pointer + position snapshot.
 */
ArenaMark ArenaGetMark(Arena *arena);

/**
 * @brief Allocates memory from the arena without zeroing it.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (uint64_t) Number of bytes to allocate.
 * @return (void*) Pointer to the allocated memory.
 */
void *ArenaPushNoZero(Arena *arena, uint64_t size);

/**
 * @brief Allocates zeroed memory from the arena.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (uint64_t) Number of bytes to allocate.
 * @return (void*) Pointer to the zeroed memory.
 */
void *ArenaPush(Arena *arena, uint64_t size);

/**
 * @brief Aligns the arena position and allocates a block of that alignment
 * size.
 * @param arena (Arena*) Pointer to the arena.
 * @param alignment (uint64_t) Alignment and allocation size in bytes.
 * @return (void*) Pointer to the aligned, zeroed memory.
 */
void *ArenaPushAligner(Arena *arena, uint64_t alignment);

/**
 * @brief Resets the arena position to a previously saved position.
 * @param arena (Arena*) Pointer to the arena.
 * @param pos (uint64_t) Position to reset to. Ignored if greater than current
 * position.
 * @note Only works within the current block. Use ArenaClear to reset across all
 * blocks.
 */
void ArenaPopTo(Arena *arena, uint64_t pos);

/**
 * @brief Resets arena to a previously captured mark.
 * @param arena (Arena*) Pointer to the arena.
 * @param mark (ArenaMark) Previously captured mark.
 */
void ArenaPopToMark(Arena *arena, ArenaMark mark);

/**
 * @brief Pops a number of bytes from the end of the arena.
 * @param arena (Arena*) Pointer to the arena.
 * @param size (uint64_t) Number of bytes to pop. Resets to 0 if size exceeds
 * current position.
 * @note Only works within the current block. Use ArenaClear to reset across all
 * blocks.
 */
void ArenaPop(Arena *arena, uint64_t size);

/**
 * @brief Resets the arena position to 0, making all memory available again.
 * @param arena (Arena*) Pointer to the arena.
 */
void ArenaClear(Arena *arena);

#endif // !ARENA_H
