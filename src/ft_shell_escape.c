#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#define OCTAL_DIGIT_MASK 0x7
#define BYTE_OCTAL_HIGH_SHIFT 6
#define BYTE_OCTAL_MID_SHIFT 3

typedef struct s_shell_out {
    t_str *dst;
    uint64_t len;
} t_shell_out;

typedef struct s_escape_state {
    bool in_single;
    bool in_ansi;
} t_escape_state;

typedef struct s_shell_analysis {
    uint64_t len;
    uint64_t escaped_len;
    char quote;
} t_shell_analysis;

static bool escape_str_(t_str *dst, const t_str *str, char quote,
                        bool pad_unquoted);
static bool append_bytes_(t_str *dst, const char *src, uint64_t len);
static bool emit_bytes_(t_shell_out *out, const char *src, uint64_t len);
static uint64_t ansi_byte_len_(unsigned char byte);
static bool append_ansi_byte_(t_str *dst, unsigned char byte);
static bool emit_ansi_byte_(t_shell_out *out, unsigned char byte);
static bool shell_escape_bytes_(t_shell_out *out, const char *src,
                                uint64_t len);
static bool escape_byte_(t_shell_out *out, t_escape_state *state,
                         unsigned char c);
static bool shell_escaped_(t_str *dst, const t_str *str);
static void analyze_shell_bytes_(const char *src, uint64_t len,
                                 t_shell_analysis *analysis);
static void fill_scan_(t_shell_scan *scan, const t_shell_analysis *analysis);
static bool needs_raw_quote_(unsigned char c, uint64_t index);
static bool is_safe_punct_(unsigned char c, uint64_t index);

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

    t_shell_out out = {.dst = NULL, .len = 0};
    shell_escape_bytes_(&out, str->str, str->len);
    return out.len;
}

char shell_quote(const t_str *str) {
    t_shell_analysis analysis;

    analyze_shell_bytes_(str->str, str->len, &analysis);
    return analysis.quote;
}

void shell_scan_cstr(const char *src, t_shell_scan *scan) {
    t_shell_analysis analysis;
    uint64_t len = 0;

    while (src[len]) {
        ++len;
    }

    analyze_shell_bytes_(src, len, &analysis);
    fill_scan_(scan, &analysis);
}

static bool escape_str_(t_str *dst, const t_str *str, char quote,
                        bool pad_unquoted) {
    if (!quote) {
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

static bool emit_bytes_(t_shell_out *out, const char *src, uint64_t len) {
    if (!out->dst) {
        out->len += len;
        return true;
    }

    if (!append_bytes_(out->dst, src, len)) {
        return false;
    }

    out->len += len;
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

static bool emit_ansi_byte_(t_shell_out *out, unsigned char byte) {
    const uint64_t len = ansi_byte_len_(byte);

    if (!out->dst) {
        out->len += len;
        return true;
    }

    if (!append_ansi_byte_(out->dst, byte)) {
        return false;
    }

    out->len += len;
    return true;
}

static bool shell_escape_bytes_(t_shell_out *out, const char *src,
                                uint64_t len) {
    t_escape_state state = {.in_single = true, .in_ansi = false};

    if (!emit_bytes_(out, "'", 1)) {
        return false;
    }

    for (uint64_t i = 0; i < len; ++i) {
        if (!escape_byte_(out, &state, (unsigned char)src[i])) {
            return false;
        }
    }

    if (state.in_single || state.in_ansi) {
        return emit_bytes_(out, "'", 1);
    }

    return true;
}

static bool escape_byte_(t_shell_out *out, t_escape_state *state,
                         unsigned char c) {
    char ch;

    if (ft_isprint(c) && c != '\'') {
        if (state->in_ansi) {
            if (!emit_bytes_(out, "'", 1)) {
                return false;
            }
            state->in_ansi = false;
        }

        if (!state->in_single) {
            if (!emit_bytes_(out, "'", 1)) {
                return false;
            }
            state->in_single = true;
        }

        ch = (char)c;
        return emit_bytes_(out, &ch, 1);
    }

    if (c == '\'') {
        if (state->in_ansi) {
            if (!emit_bytes_(out, "'", 1)) {
                return false;
            }
            state->in_ansi = false;
        }

        if (state->in_single) {
            return emit_bytes_(out, "'\\''", 4);
        }

        return emit_bytes_(out, "\\'", 2);
    }

    if (state->in_single) {
        if (!emit_bytes_(out, "'", 1)) {
            return false;
        }
        state->in_single = false;
    }

    if (!state->in_ansi) {
        if (!emit_bytes_(out, "$'", 2)) {
            return false;
        }
        state->in_ansi = true;
    }

    return emit_ansi_byte_(out, c);
}

static bool shell_escaped_(t_str *dst, const t_str *str) {
    t_shell_out out;

    out.dst = dst;
    out.len = 0;
    return shell_escape_bytes_(&out, str->str, str->len);
}

static void analyze_shell_bytes_(const char *src, uint64_t len,
                                 t_shell_analysis *analysis) {
    bool needs_quote = false;
    bool has_single = false;
    bool can_use_double = true;
    bool has_non_print = false;
    t_shell_out escaped;

    escaped.dst = NULL;
    escaped.len = 0;
    shell_escape_bytes_(&escaped, src, len);

    for (uint64_t i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char)src[i];

        if (!ft_isprint(c)) {
            has_non_print = true;
        }

        if (needs_raw_quote_(c, i)) {
            needs_quote = true;
            if (c != ' ' && c != '\'') {
                can_use_double = false;
            }
        }

        if (c == '\'') {
            has_single = true;
        }
    }

    analysis->len = len;
    analysis->escaped_len = escaped.len;

    if (has_non_print) {
        analysis->quote = '\'';
    } else if (!needs_quote) {
        analysis->quote = '\0';
    } else if (has_single && can_use_double) {
        analysis->quote = '"';
    } else {
        analysis->quote = '\'';
    }
}

static void fill_scan_(t_shell_scan *scan, const t_shell_analysis *analysis) {
    scan->len = analysis->len;
    scan->quote = analysis->quote;

    if (analysis->quote == '\0') {
        scan->display_len = analysis->len;
        scan->padded_display_len = analysis->len + 1;
    } else if (analysis->quote == '"') {
        scan->display_len = analysis->len + 2;
        scan->padded_display_len = scan->display_len;
    } else {
        scan->display_len = analysis->escaped_len;
        scan->padded_display_len = analysis->escaped_len;
    }
}

static bool needs_raw_quote_(unsigned char c, uint64_t index) {
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
