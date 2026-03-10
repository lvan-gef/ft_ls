#include <stdint.h>

#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_entry.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

t_entry *escape_entry(Arena *arena, t_entry *entry) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(entry);

    if (entry->is_escaped) {
        return entry;
    }

    if (!entry->quoted->len) {
        entry->is_escaped = true;
        return entry;
    }

    if (entry->quoted->str[0] == '"') {
        entry->is_escaped = true;
        return entry;
    }

    t_str *new_str = escape_str(arena, entry->name);
    if (!new_str) {
        return entry;
    }

    entry->name = new_str;
    entry->is_escaped = true;
    return entry;
}

t_str *escape_str(Arena *arena, t_str *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);

    uint64_t single_count = 0;
    for (uint64_t index = 0; index < str->len; ++index) {
        if (str->str[index] == '\'') {
            ++single_count;
        }
    }

    t_str *new_str = init_str(arena, str->len + (single_count * 3));
    if (!new_str) {
        return NULL;
    }

    for (uint64_t index = 0; index < str->len; ++index) {
        if (str->str[index] == '\'') {
            ft_memcpy(new_str->str + new_str->len, "'\\''", 4);
            new_str->len += 4;
            continue;
        }

        new_str->str[new_str->len] = str->str[index];
        ++new_str->len;
    }

    new_str->str[new_str->len] = '\0';
    return new_str;
}
