#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#include "ft_shell_escape.h"

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
    char quote;
    bool needs_quote;
    bool has_single;
    bool can_use_double;
    bool has_non_print;
} t_shell_analysis;

static void escape_str_(t_str *dst, const t_str *str, char quote,
                        bool pad_unquoted);
static void append_bytes_(t_str *dst, const char *src, uint64_t len);
static void emit_bytes_(t_shell_out *out, const char *src, uint64_t len);
static void emit_ansi_byte_(t_shell_out *out, unsigned char byte);
static void enter_single_(t_shell_out *out, t_escape_state *state);
static void leave_single_(t_shell_out *out, t_escape_state *state);
static void enter_ansi_(t_shell_out *out, t_escape_state *state);
static void leave_ansi_(t_shell_out *out, t_escape_state *state);
static void shell_escape_bytes_(t_shell_out *out, const char *src,
                                uint64_t len);
static void escape_byte_(t_shell_out *out, t_escape_state *state,
                         unsigned char c);
static uint64_t escaped_len_(const char *src, uint64_t len, char quote,
                             bool pad_unquoted);
static uint64_t single_quoted_len_(const char *src, uint64_t len);
static void scan_shell_(const char *src, uint64_t len, t_shell_scan *scan);
static void fill_scan_(t_shell_scan *scan, const char *src,
                       const t_shell_analysis *analysis);
static bool needs_raw_quote_(unsigned char c, uint64_t index);
static bool is_safe_punct_(unsigned char c, uint64_t index);
static void analyze_shell_span_(const char *src, uint64_t len,
                                t_shell_analysis *analysis);
static void analyze_shell_byte_(t_shell_analysis *analysis, unsigned char c,
                                uint64_t index);
static void init_analysis_(t_shell_analysis *analysis);
static char quote_from_analysis_(const t_shell_analysis *analysis);

void shell_scan_str(const t_str *str, t_shell_scan *scan) {
    scan_shell_(str->str + str->pos, str->len, scan);
}

uint64_t shell_escaped_len(const t_str *str, const char quote,
                           const bool pad_unquoted) {
    return escaped_len_(str->str + str->pos, str->len, quote, pad_unquoted);
}

bool shell_escape_append(t_str *dst, const t_str *str, const char quote,
                         const bool pad_unquoted) {
    if (dst->cap - 1 - dst->len < shell_escaped_len(str, quote, pad_unquoted)) {
        return false;
    }

    escape_str_(dst, str, quote, pad_unquoted);
    return true;
}

t_str *shell_escape_str(const t_str *str, const char quote) {
    const uint64_t escaped_len = shell_escaped_len(str, quote, false);
    t_str *new_str = str_new(escaped_len);

    if (!new_str) {
        return NULL;
    }

    escape_str_(new_str, str, quote, false);
    return new_str;
}

static void escape_str_(t_str *dst, const t_str *str, const char quote,
                        const bool pad_unquoted) {
    if (!quote) {
        if (pad_unquoted) {
            append_bytes_(dst, " ", 1);
        }

        append_bytes_(dst, str->str + str->pos, str->len);
        return;
    }

    if (quote == '"') {
        append_bytes_(dst, "\"", 1);
        append_bytes_(dst, str->str + str->pos, str->len);
        append_bytes_(dst, "\"", 1);
        return;
    }

    shell_escape_bytes_(&(t_shell_out){.dst = dst, .len = 0},
                        str->str + str->pos, str->len);
}

static void append_bytes_(t_str *dst, const char *src, const uint64_t len) {
    ft_memcpy(dst->str + dst->len, src, (size_t)len);
    dst->len += len;
    dst->str[dst->len] = '\0';
}

static void emit_bytes_(t_shell_out *out, const char *src, const uint64_t len) {
    if (!out->dst) {
        out->len += len;
        return;
    }

    append_bytes_(out->dst, src, len);
    out->len += len;
}

static void emit_ansi_byte_(t_shell_out *out, const unsigned char byte) {
    char repr[4];
    uint64_t len = 2;

    switch (byte) {
        case '\a':
            repr[0] = '\\';
            repr[1] = 'a';
            break;
        case '\b':
            repr[0] = '\\';
            repr[1] = 'b';
            break;
        case '\t':
            repr[0] = '\\';
            repr[1] = 't';
            break;
        case '\n':
            repr[0] = '\\';
            repr[1] = 'n';
            break;
        case '\v':
            repr[0] = '\\';
            repr[1] = 'v';
            break;
        case '\f':
            repr[0] = '\\';
            repr[1] = 'f';
            break;
        case '\r':
            repr[0] = '\\';
            repr[1] = 'r';
            break;
        default:
            len = 4;
            repr[0] = '\\';
            repr[1] = (char)('0' + ((byte >> BYTE_OCTAL_HIGH_SHIFT) &
                                    OCTAL_DIGIT_MASK));
            repr[2] = (char)('0' + ((byte >> BYTE_OCTAL_MID_SHIFT) &
                                    OCTAL_DIGIT_MASK));
            repr[3] = (char)('0' + (byte & OCTAL_DIGIT_MASK));
            break;
    }

    emit_bytes_(out, repr, len);
}

static void enter_single_(t_shell_out *out, t_escape_state *state) {
    if (state->in_single) {
        return;
    }

    emit_bytes_(out, "'", 1);
    state->in_single = true;
}

static void leave_single_(t_shell_out *out, t_escape_state *state) {
    if (!state->in_single) {
        return;
    }

    emit_bytes_(out, "'", 1);
    state->in_single = false;
}

static void enter_ansi_(t_shell_out *out, t_escape_state *state) {
    if (state->in_ansi) {
        return;
    }

    emit_bytes_(out, "$'", 2);
    state->in_ansi = true;
}

static void leave_ansi_(t_shell_out *out, t_escape_state *state) {
    if (!state->in_ansi) {
        return;
    }

    emit_bytes_(out, "'", 1);
    state->in_ansi = false;
}

static void shell_escape_bytes_(t_shell_out *out, const char *src,
                                const uint64_t len) {
    t_escape_state state = {.in_single = true, .in_ansi = false};

    emit_bytes_(out, "'", 1);
    for (uint64_t i = 0; i < len; ++i) {
        escape_byte_(out, &state, (unsigned char)src[i]);
    }

    if (state.in_single || state.in_ansi) {
        emit_bytes_(out, "'", 1);
    }
}

static void escape_byte_(t_shell_out *out, t_escape_state *state,
                         const unsigned char c) {
    if (ft_isprint(c) && c != '\'') {
        leave_ansi_(out, state);
        enter_single_(out, state);
        const char ch = (char)c;
        emit_bytes_(out, &ch, 1);
        return;
    }

    if (c == '\'') {
        leave_ansi_(out, state);
        emit_bytes_(out, state->in_single ? "'\\''" : "\\'",
                    state->in_single ? 4U : 2U);
        return;
    }

    leave_single_(out, state);
    enter_ansi_(out, state);
    emit_ansi_byte_(out, c);
}

static uint64_t escaped_len_(const char *src, const uint64_t len,
                             const char quote, const bool pad_unquoted) {
    if (quote == '\0') {
        return len + (pad_unquoted ? 1 : 0);
    }

    if (quote == '"') {
        return len + 2;
    }

    return single_quoted_len_(src, len);
}

static uint64_t single_quoted_len_(const char *src, const uint64_t len) {
    t_shell_out out = {.dst = NULL, .len = 0};

    shell_escape_bytes_(&out, src, len);
    return out.len;
}

static void scan_shell_(const char *src, const uint64_t len,
                        t_shell_scan *scan) {
    t_shell_analysis analysis;

    analyze_shell_span_(src, len, &analysis);
    fill_scan_(scan, src, &analysis);
}

static void fill_scan_(t_shell_scan *scan, const char *src,
                       const t_shell_analysis *analysis) {
    const uint64_t display_len =
        escaped_len_(src, analysis->len, analysis->quote, false);

    scan->len = analysis->len;
    scan->quote = analysis->quote;
    scan->display_len = display_len;
    scan->padded_display_len =
        analysis->quote == '\0' ? display_len + 1 : display_len;
}

static bool needs_raw_quote_(const unsigned char c, const uint64_t index) {
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

static bool is_safe_punct_(const unsigned char c, const uint64_t index) {
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

static void analyze_shell_span_(const char *src, const uint64_t len,
                                t_shell_analysis *analysis) {
    init_analysis_(analysis);
    analysis->len = len;
    for (uint64_t i = 0; i < len; ++i) {
        analyze_shell_byte_(analysis, (unsigned char)src[i], i);
    }

    analysis->quote = quote_from_analysis_(analysis);
}

static void analyze_shell_byte_(t_shell_analysis *analysis,
                                const unsigned char c, const uint64_t index) {
    if (!ft_isprint(c)) {
        analysis->has_non_print = true;
    }

    if (needs_raw_quote_(c, index)) {
        analysis->needs_quote = true;
        if (c != ' ' && c != '\'') {
            analysis->can_use_double = false;
        }
    }

    if (c == '\'') {
        analysis->has_single = true;
    }
}

static void init_analysis_(t_shell_analysis *analysis) {
    analysis->len = 0;
    analysis->quote = '\0';
    analysis->needs_quote = false;
    analysis->has_single = false;
    analysis->can_use_double = true;
    analysis->has_non_print = false;
}

static char quote_from_analysis_(const t_shell_analysis *analysis) {
    if (analysis->has_non_print) {
        return '\'';
    }

    if (!analysis->needs_quote) {
        return '\0';
    }

    if (analysis->has_single && analysis->can_use_double) {
        return '"';
    }

    return '\'';
}
