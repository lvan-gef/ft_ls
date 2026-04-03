#include <stdint.h>

#include "../include/ft_assert.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

static bool is_alpha_(unsigned char c);
static bool is_digit_(unsigned char c);
static bool is_name_start_(unsigned char c);
static bool is_name_continue_(unsigned char c);
static bool is_printable_ascii_(unsigned char c);
static bool is_raw_safe_punct_(unsigned char c, uint64_t index);
static bool needs_raw_quote_(unsigned char c, uint64_t index,
                             bool *assignment_candidate);
static bool write_ansi_byte_(void *ctx, ft_write_mem_fn writer,
                             unsigned char byte);
static bool write_ansi_c_segment_(void *ctx, ft_write_mem_fn writer,
                                  const char *src, uint64_t len);
static bool write_single_shell_escaped_(void *ctx, ft_write_mem_fn writer,
                                        const t_str *str);
static bool count_writer_(void *ctx, const char *src, uint64_t len);
static bool str_writer_(void *ctx, const char *src, uint64_t len);

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}

char shell_quote_style(const t_str *str) {
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    bool needs_quote = false;
    bool has_single = false;
    bool can_use_double = true;
    bool assignment_candidate = true;

    for (uint64_t index = 0; index < str->len; ++index) {
        const unsigned char c = (unsigned char)str->str[index];
        if (!is_printable_ascii_(c)) {
            return '\'';
        }

        if (needs_raw_quote_(c, index, &assignment_candidate)) {
            needs_quote = true;
            if (c != ' ' && c != '\'') {
                can_use_double = false;
            }
        }

        if (c == '\'') {
            has_single = true;
        }
    }

    if (!needs_quote) {
        return '\0';
    }

    if (has_single && can_use_double) {
        return '"';
    }

    return '\'';
}

bool has_shell_quote_char(const t_str *str) {
    return shell_quote_style(str) != '\0';
}

uint64_t shell_display_len(const t_str *str, char quote, bool pad_unquoted) {
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    uint64_t counter = 0;

    const bool ok =
        write_shell_escaped(&counter, count_writer_, str, quote, pad_unquoted);
    ASSERT_TRUE(ok);
    return counter;
}

bool write_shell_escaped(void *ctx, ft_write_mem_fn writer, const t_str *str,
                         char quote, bool pad_unquoted) {
    ASSERT_NOTNULL(writer);
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    if (quote == '\0') {
        if (pad_unquoted && !writer(ctx, " ", 1)) {
            return false;
        }

        return writer(ctx, str->str, str->len);
    }

    if (quote == '"') {
        if (!writer(ctx, "\"", 1) || !writer(ctx, str->str, str->len)) {
            return false;
        }

        return writer(ctx, "\"", 1);
    }

    ASSERT_EQ(quote, '\'');
    return write_single_shell_escaped_(ctx, writer, str);
}

t_str *shell_escape_str(free_list *fl, const t_str *str, char quote) {
    ASSERT_NOTNULL(fl);
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    const uint64_t escaped_len = shell_display_len(str, quote, false);
    t_str *new_str = init_str(fl, escaped_len);
    if (!new_str) {
        return NULL;
    }

    if (!write_shell_escaped(new_str, str_writer_, str, quote, false)) {
        free_str(fl, new_str);
        return NULL;
    }

    return new_str;
}

static bool is_alpha_(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_digit_(unsigned char c) {
    return c >= '0' && c <= '9';
}

static bool is_name_start_(unsigned char c) {
    return is_alpha_(c) || c == '_';
}

static bool is_name_continue_(unsigned char c) {
    return is_name_start_(c) || is_digit_(c);
}

static bool is_printable_ascii_(unsigned char c) {
    return c >= 0x20 && c <= 0x7e;
}

static bool is_raw_safe_punct_(unsigned char c, uint64_t index) {
    switch (c) {
        case '_':
        case '-':
        case '.':
        case '/':
        case '+':
        case ':':
        case ',':
        case '@':
        case '%':
        case '{':
        case '}': return true;
        case '#':
        case '~': return index != 0;
        default: return false;
    }
}

static bool needs_raw_quote_(unsigned char c, uint64_t index,
                             bool *assignment_candidate) {
    bool needs_quote = false;

    ASSERT_NOTNULL(assignment_candidate);

    switch (c) {
        case ' ':
        case '\'':
        case '"':
        case '!':
        case '&':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case '?':
        case '<':
        case '>':
        case '|':
        case ';':
        case '$':
        case '`':
        case '\\':
        case '^': needs_quote = true; break;
        case '=': needs_quote = true; break;
        default:
            needs_quote =
                !is_alpha_(c) && !is_digit_(c) && !is_raw_safe_punct_(c, index);
            break;
    }

    if (*assignment_candidate) {
        if (index == 0) {
            *assignment_candidate = is_name_start_(c);
        } else {
            *assignment_candidate = is_name_continue_(c);
        }
    }

    if (c == '/') {
        *assignment_candidate = false;
    }

    return needs_quote;
}

static bool write_ansi_byte_(void *ctx, ft_write_mem_fn writer,
                             unsigned char byte) {
    char octal[4];

    switch (byte) {
        case '\a': return writer(ctx, "\\a", 2);
        case '\b': return writer(ctx, "\\b", 2);
        case '\t': return writer(ctx, "\\t", 2);
        case '\n': return writer(ctx, "\\n", 2);
        case '\v': return writer(ctx, "\\v", 2);
        case '\f': return writer(ctx, "\\f", 2);
        case '\r': return writer(ctx, "\\r", 2);
        default: break;
    }

    octal[0] = '\\';
    octal[1] = (char)('0' + ((byte >> 6) & 0x7));
    octal[2] = (char)('0' + ((byte >> 3) & 0x7));
    octal[3] = (char)('0' + (byte & 0x7));
    return writer(ctx, octal, 4);
}

static bool write_ansi_c_segment_(void *ctx, ft_write_mem_fn writer,
                                  const char *src, uint64_t len) {
    ASSERT_NOTNULL(src);

    if (!writer(ctx, "$'", 2)) {
        return false;
    }

    for (uint64_t index = 0; index < len; ++index) {
        if (!write_ansi_byte_(ctx, writer, (unsigned char)src[index])) {
            return false;
        }
    }

    return writer(ctx, "'", 1);
}

static bool write_single_shell_escaped_(void *ctx, ft_write_mem_fn writer,
                                        const t_str *str) {
    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);

    bool in_single = true;
    if (!writer(ctx, "'", 1)) {
        return false;
    }

    for (uint64_t index = 0; index < str->len;) {
        const unsigned char c = (unsigned char)str->str[index];
        if (is_printable_ascii_(c) && c != '\'') {
            if (!in_single) {
                if (!writer(ctx, "'", 1)) {
                    return false;
                }
                in_single = true;
            }

            if (!writer(ctx, str->str + index, 1)) {
                return false;
            }
            ++index;
            continue;
        }

        if (c == '\'') {
            if (!writer(ctx, "'\\''", 4)) {
                return false;
            }
            in_single = true;
            ++index;
            continue;
        }

        if (in_single) {
            if (!writer(ctx, "'", 1)) {
                return false;
            }
            in_single = false;
        }

        const uint64_t start = index;
        while (index < str->len) {
            const unsigned char next = (unsigned char)str->str[index];
            if (next == '\'' || is_printable_ascii_(next)) {
                break;
            }
            ++index;
        }

        if (!write_ansi_c_segment_(ctx, writer, str->str + start,
                                   index - start)) {
            return false;
        }
    }

    if (!in_single) {
        return true;
    }

    return writer(ctx, "'", 1);
}

static bool count_writer_(void *ctx, const char *src, uint64_t len) {
    ASSERT_NOTNULL(ctx);
    (void)src;

    *(uint64_t *)ctx += len;
    return true;
}

static bool str_writer_(void *ctx, const char *src, uint64_t len) {
    t_str *str = ctx;

    ASSERT_NOTNULL(str);
    ASSERT_NOTNULL(str->str);
    ASSERT_NOTNULL(src);
    ASSERT_LE(str->len + len, str->cap - 1);

    ft_memcpy(str->str + str->len, src, (size_t)len);
    str->len += len;
    str->str[str->len] = '\0';
    return true;
}
