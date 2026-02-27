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
 * @brief Captures the current arena state for later rollback.
 * @param arena (Arena*) Pointer to the arena.
 * @return (ArenaMark) Block pointer + position snapshot.
 */
ArenaMark ArenaGetMark(const Arena *arena);

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
 * @brief Resets arena to a previously captured mark.
 * @param arena (Arena*) Pointer to the arena.
 * @param mark (ArenaMark) Previously captured mark.
 */
void ArenaPopToMark(Arena *arena, ArenaMark mark);

/**
 * @brief Resets the arena position to 0, making all memory available again.
 * @param arena (Arena*) Pointer to the arena.
 */
void ArenaClear(Arena *arena);

#endif // !ARENA_H
