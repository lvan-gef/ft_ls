#include <stdint.h>

#include "../include/ft_helper.h"
#include "../include/ft_entry.h"
#include "../include/ft_arena.h"
#include "../include/ft_assert.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static t_str *escape_str_(Arena *arena, t_str *str);

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

t_entry *escape_seq_entry(Arena *arena, t_entry *entry) {
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

    t_str *new_str = escape_str_(arena, entry->name);
    if (!new_str) {
        return entry;
    }

    entry->name = new_str;
    entry->is_escaped = true;
    return entry;
}

static t_str *escape_str_(Arena *arena, t_str *str) {
    ASSERT_NOTNULL(arena);
    ASSERT_NOTNULL(str);

    uint64_t single_count = 0;
    const uint64_t cur_pos = str->pos;
    while (has_next_str(str)) {
        const char lttr = next_str(str);
        if (lttr == '\'') {
            ++single_count;
        }
    }
    str->pos = cur_pos;

    t_str *new_str = init_str(arena, str->len + (single_count * 3));
    if (!new_str) {
        return NULL;
    }

    const char *quote = ft_memchr(str->str, '\'', str->len);
    while (quote) {
        uint64_t len = (uint64_t)(quote - str->str);
        ft_memcpy(new_str->str + new_str->len, str->str + str->pos, len);
        new_str->len += len;

        ft_memcpy(new_str->str + new_str->len, "'\\'", 3);
        new_str->len += 3;
        str->pos += len;
        ++quote;
        quote = ft_memchr(quote, '\'', str->len);
    }

    ft_memcpy(new_str->str + new_str->len, str->str + str->pos, str->len - str->pos);
    new_str->len += str->len - str->pos;
    new_str->str[new_str->len] = '\0';
    return new_str;
}
