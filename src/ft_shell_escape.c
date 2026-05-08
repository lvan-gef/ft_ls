#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#define OCTAL_DIGIT_MASK 0x7
#define BYTE_OCTAL_HIGH_SHIFT 6
#define BYTE_OCTAL_MID_SHIFT 3

static bool escape_str_(t_str *dst, const t_str *str, char quote,
                        bool pad_unquoted);
static bool append_bytes_(t_str *dst, const char *src, uint64_t len);
static uint64_t ansi_byte_len_(unsigned char byte);
static uint64_t ansi_len_(const char *src, uint64_t len);
static uint64_t escaped_len_(const t_str *str);
static bool append_ansi_byte_(t_str *dst, unsigned char byte);
static bool append_ansi_segment_(t_str *dst, const char *src, uint64_t len);
static bool shell_escaped_(t_str *dst, const t_str *str);
static bool needs_raw_quote_(unsigned char c, uint64_t index, bool *candidate);
static bool is_safe_punct_(unsigned char c, uint64_t index);
static bool name_start_(unsigned char c);
static bool name_continue_(unsigned char c);

bool escaped_out(t_str *dst, const t_str *str, char quote, bool pad_unquoted) {
    const uint64_t need = shell_display_len(str, quote, pad_unquoted);
    if (need > dst->cap - 1) {
        return false;
    }

    if (dst->cap - 1 - dst->len < need && !flush_str(dst)) {
        return false;
    }

    return escape_str_(dst, str, quote, pad_unquoted);
}

t_str *shell_escape_str(free_list *fl, const t_str *str, char quote) {
    const uint64_t escaped_len = shell_display_len(str, quote, false);
    t_str *new_str = init_str(fl, escaped_len);
    if (!new_str) {
        return NULL;
    }

    if (!escape_str_(new_str, str, quote, false)) {
        free_str(fl, new_str);
        return NULL;
    }

    return new_str;
}

uint64_t shell_display_len(const t_str *str, char quote, bool pad_unquoted) {
    if (quote == '\0') {
        return str->len + (pad_unquoted ? 1 : 0);
    }

    if (quote == '"') {
        return str->len + 2;
    }

    return escaped_len_(str);
}

char shell_quote(const t_str *str) {
    bool needs_quote = false;
    bool has_single = false;
    bool can_use_double = true;
    bool assignment_candidate = true;

    for (uint64_t index = 0; index < str->len; ++index) {
        const unsigned char c = (unsigned char)str->str[index];
        if (!ft_isprint(c)) {
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

static bool escape_str_(t_str *dst, const t_str *str, char quote,
                        bool pad_unquoted) {
    if (quote == '\0') {
        if (pad_unquoted && !append_bytes_(dst, " ", 1)) {
            return false;
        }
        return append_bytes_(dst, str->str, str->len);
    }

    if (quote == '"') {
        if (!append_bytes_(dst, "\"", 1) ||
            !append_bytes_(dst, str->str, str->len)) {
            return false;
        }
        return append_bytes_(dst, "\"", 1);
    }

    return shell_escaped_(dst, str);
}

static bool append_bytes_(t_str *dst, const char *src, uint64_t len) {
    ft_memcpy(dst->str + dst->len, src, (size_t)len);
    dst->len += len;
    dst->str[dst->len] = '\0';
    return true;
}

static uint64_t ansi_byte_len_(unsigned char byte) {
    switch (byte) {
        case '\a':
        case '\b':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r': return 2;
        default: return 4;
    }
}

static uint64_t ansi_len_(const char *src, uint64_t len) {
    uint64_t out = 3; /* $'  + closing ' */
    for (uint64_t i = 0; i < len; ++i) {
        out += ansi_byte_len_((unsigned char)src[i]);
    }

    return out;
}

static uint64_t escaped_len_(const t_str *str) {
    uint64_t out = 1; /* opening ' */
    bool in_single = true;

    for (uint64_t index = 0; index < str->len;) {
        const unsigned char c = (unsigned char)str->str[index];

        if (ft_isprint(c) && c != '\'') {
            if (!in_single) {
                ++out;
                in_single = true;
            }
            ++out;
            ++index;
            continue;
        }

        if (c == '\'') {
            out += 4; /* '\'' */
            in_single = true;
            ++index;
            continue;
        }

        if (in_single) {
            ++out;
            in_single = false;
        }

        const uint64_t start = index;
        while (index < str->len) {
            const unsigned char next = (unsigned char)str->str[index];
            if (next == '\'' || ft_isprint(next)) {
                break;
            }

            ++index;
        }

        out += ansi_len_(str->str + start, index - start);
    }

    if (in_single) {
        ++out;
    }

    return out;
}

static bool append_ansi_byte_(t_str *dst, unsigned char byte) {
    char octal[4];
    switch (byte) {
        case '\a': return append_bytes_(dst, "\\a", 2);
        case '\b': return append_bytes_(dst, "\\b", 2);
        case '\t': return append_bytes_(dst, "\\t", 2);
        case '\n': return append_bytes_(dst, "\\n", 2);
        case '\v': return append_bytes_(dst, "\\v", 2);
        case '\f': return append_bytes_(dst, "\\f", 2);
        case '\r': return append_bytes_(dst, "\\r", 2);
        default: break;
    }

    octal[0] = '\\';
    octal[1] =
        (char)('0' + ((byte >> BYTE_OCTAL_HIGH_SHIFT) & OCTAL_DIGIT_MASK));
    octal[2] =
        (char)('0' + ((byte >> BYTE_OCTAL_MID_SHIFT) & OCTAL_DIGIT_MASK));
    octal[3] = (char)('0' + (byte & OCTAL_DIGIT_MASK));

    return append_bytes_(dst, octal, 4);
}

static bool append_ansi_segment_(t_str *dst, const char *src, uint64_t len) {
    if (!append_bytes_(dst, "$'", 2)) {
        return false;
    }

    for (uint64_t i = 0; i < len; ++i) {
        if (!append_ansi_byte_(dst, (unsigned char)src[i])) {
            return false;
        }
    }

    return append_bytes_(dst, "\'", 1);
}

static bool shell_escaped_(t_str *dst, const t_str *str) {
    bool in_single = true;

    if (!append_bytes_(dst, "\'", 1)) {
        return false;
    }

    for (uint64_t index = 0; index < str->len;) {
        const unsigned char c = (unsigned char)str->str[index];

        if (ft_isprint(c) && c != '\'') {
            if (!in_single) {
                if (!append_bytes_(dst, "\'", 1)) {
                    return false;
                }
                in_single = true;
            }

            if (!append_bytes_(dst, str->str + index, 1)) {
                return false;
            }

            ++index;
            continue;
        }

        if (c == '\'') {
            if (!append_bytes_(dst, "'\\''", 4)) {
                return false;
            }

            in_single = true;
            ++index;
            continue;
        }

        if (in_single) {
            if (!append_bytes_(dst, "\'", 1)) {
                return false;
            }

            in_single = false;
        }

        const uint64_t start = index;
        while (index < str->len) {
            const unsigned char next = (unsigned char)str->str[index];
            if (next == '\'' || ft_isprint(next)) {
                break;
            }

            ++index;
        }

        if (!append_ansi_segment_(dst, str->str + start, index - start)) {
            return false;
        }
    }

    if (!in_single) {
        return true;
    }

    return append_bytes_(dst, "\'", 1);
}

static bool needs_raw_quote_(unsigned char c, uint64_t index, bool *candidate) {
    bool needs_quote = false;

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
                !ft_isalpha(c) && !ft_isdigit(c) && !is_safe_punct_(c, index);
            break;
    }

    if (*candidate) {
        if (index == 0) {
            *candidate = name_start_(c);
        } else {
            *candidate = name_continue_(c);
        }
    }

    if (c == '/') {
        *candidate = false;
    }

    return needs_quote;
}

static bool is_safe_punct_(unsigned char c, uint64_t index) {
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

static bool name_start_(unsigned char c) {
    return ft_isalpha(c) || c == '_';
}

static bool name_continue_(unsigned char c) {
    return name_start_(c) || ft_isdigit(c);
}
