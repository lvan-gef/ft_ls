#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_printer_helper.h"
#include "../include/ft_shell_escape.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

#define OCTAL_DIGIT_MASK 0x7
#define BYTE_OCTAL_HIGH_SHIFT 6
#define BYTE_OCTAL_MID_SHIFT 3

#define SHELL_CLASS_SAFE 0x1u
#define SHELL_CLASS_ALNUM 0x2u

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
static void analyze_shell_cstr_(const char *src, t_shell_analysis *analysis);
static void analyze_shell_span_(const char *src, uint64_t len,
                                t_shell_analysis *analysis);
static char quote_from_analysis_(const t_shell_analysis *analysis);
static void fill_scan_(t_shell_scan *scan, const char *src,
                       const t_shell_analysis *analysis);
static bool needs_raw_quote_(unsigned char c, uint64_t index);

static const unsigned char g_shell_class_table_[256] = {
    ['0'] = SHELL_CLASS_ALNUM,
    ['1'] = SHELL_CLASS_ALNUM,
    ['2'] = SHELL_CLASS_ALNUM,
    ['3'] = SHELL_CLASS_ALNUM,
    ['4'] = SHELL_CLASS_ALNUM,
    ['5'] = SHELL_CLASS_ALNUM,
    ['6'] = SHELL_CLASS_ALNUM,
    ['7'] = SHELL_CLASS_ALNUM,
    ['8'] = SHELL_CLASS_ALNUM,
    ['9'] = SHELL_CLASS_ALNUM,
    ['A'] = SHELL_CLASS_ALNUM,
    ['B'] = SHELL_CLASS_ALNUM,
    ['C'] = SHELL_CLASS_ALNUM,
    ['D'] = SHELL_CLASS_ALNUM,
    ['E'] = SHELL_CLASS_ALNUM,
    ['F'] = SHELL_CLASS_ALNUM,
    ['G'] = SHELL_CLASS_ALNUM,
    ['H'] = SHELL_CLASS_ALNUM,
    ['I'] = SHELL_CLASS_ALNUM,
    ['J'] = SHELL_CLASS_ALNUM,
    ['K'] = SHELL_CLASS_ALNUM,
    ['L'] = SHELL_CLASS_ALNUM,
    ['M'] = SHELL_CLASS_ALNUM,
    ['N'] = SHELL_CLASS_ALNUM,
    ['O'] = SHELL_CLASS_ALNUM,
    ['P'] = SHELL_CLASS_ALNUM,
    ['Q'] = SHELL_CLASS_ALNUM,
    ['R'] = SHELL_CLASS_ALNUM,
    ['S'] = SHELL_CLASS_ALNUM,
    ['T'] = SHELL_CLASS_ALNUM,
    ['U'] = SHELL_CLASS_ALNUM,
    ['V'] = SHELL_CLASS_ALNUM,
    ['W'] = SHELL_CLASS_ALNUM,
    ['X'] = SHELL_CLASS_ALNUM,
    ['Y'] = SHELL_CLASS_ALNUM,
    ['Z'] = SHELL_CLASS_ALNUM,
    ['_'] = SHELL_CLASS_SAFE,
    ['a'] = SHELL_CLASS_ALNUM,
    ['b'] = SHELL_CLASS_ALNUM,
    ['c'] = SHELL_CLASS_ALNUM,
    ['d'] = SHELL_CLASS_ALNUM,
    ['e'] = SHELL_CLASS_ALNUM,
    ['f'] = SHELL_CLASS_ALNUM,
    ['g'] = SHELL_CLASS_ALNUM,
    ['h'] = SHELL_CLASS_ALNUM,
    ['i'] = SHELL_CLASS_ALNUM,
    ['j'] = SHELL_CLASS_ALNUM,
    ['k'] = SHELL_CLASS_ALNUM,
    ['l'] = SHELL_CLASS_ALNUM,
    ['m'] = SHELL_CLASS_ALNUM,
    ['n'] = SHELL_CLASS_ALNUM,
    ['o'] = SHELL_CLASS_ALNUM,
    ['p'] = SHELL_CLASS_ALNUM,
    ['q'] = SHELL_CLASS_ALNUM,
    ['r'] = SHELL_CLASS_ALNUM,
    ['s'] = SHELL_CLASS_ALNUM,
    ['t'] = SHELL_CLASS_ALNUM,
    ['u'] = SHELL_CLASS_ALNUM,
    ['v'] = SHELL_CLASS_ALNUM,
    ['w'] = SHELL_CLASS_ALNUM,
    ['x'] = SHELL_CLASS_ALNUM,
    ['y'] = SHELL_CLASS_ALNUM,
    ['z'] = SHELL_CLASS_ALNUM,
    ['#'] = SHELL_CLASS_SAFE,
    ['%'] = SHELL_CLASS_SAFE,
    ['+'] = SHELL_CLASS_SAFE,
    [','] = SHELL_CLASS_SAFE,
    ['-'] = SHELL_CLASS_SAFE,
    ['.'] = SHELL_CLASS_SAFE,
    ['/'] = SHELL_CLASS_SAFE,
    [':'] = SHELL_CLASS_SAFE,
    ['@'] = SHELL_CLASS_SAFE,
    ['{'] = SHELL_CLASS_SAFE,
    ['}'] = SHELL_CLASS_SAFE,
    ['~'] = SHELL_CLASS_SAFE,
};

bool escaped_out(t_str *dst, const t_str *str, char quote, bool pad_unquoted) {
    const uint64_t need = shell_display_len(str, quote, pad_unquoted);
    return escaped_out_len(dst, str, quote, need, pad_unquoted);
}

bool escaped_out_len(t_str *dst, const t_str *str, char quote, uint64_t need,
                     bool pad_unquoted) {
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

    analyze_shell_span_(str->str, str->len, &analysis);
    return quote_from_analysis_(&analysis);
}

void shell_scan_str(const t_str *str, t_shell_scan *scan) {
    t_shell_analysis analysis;

    analyze_shell_span_(str->str, str->len, &analysis);
    fill_scan_(scan, str->str, &analysis);
}

void shell_scan_cstr(const char *src, t_shell_scan *scan) {
    t_shell_analysis analysis;

    analyze_shell_cstr_(src, &analysis);
    fill_scan_(scan, src, &analysis);
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

static void analyze_shell_cstr_(const char *src, t_shell_analysis *analysis) {
    uint64_t len;

    analysis->len = 0;
    analysis->needs_quote = false;
    analysis->has_single = false;
    analysis->can_use_double = true;
    analysis->has_non_print = false;
    for (len = 0; src[len]; ++len) {
        const unsigned char c = (unsigned char)src[len];

        if (!ft_isprint(c)) {
            analysis->has_non_print = true;
        }
        if (needs_raw_quote_(c, len)) {
            analysis->needs_quote = true;
            if (c != ' ' && c != '\'') {
                analysis->can_use_double = false;
            }
        }
        if (c == '\'') {
            analysis->has_single = true;
        }
    }
    analysis->len = len;
    analysis->quote = quote_from_analysis_(analysis);
}

static void analyze_shell_span_(const char *src, uint64_t len,
                                t_shell_analysis *analysis) {
    analysis->len = len;
    analysis->needs_quote = false;
    analysis->has_single = false;
    analysis->can_use_double = true;
    analysis->has_non_print = false;
    for (uint64_t i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char)src[i];

        if (!ft_isprint(c)) {
            analysis->has_non_print = true;
        }
        if (needs_raw_quote_(c, i)) {
            analysis->needs_quote = true;
            if (c != ' ' && c != '\'') {
                analysis->can_use_double = false;
            }
        }
        if (c == '\'') {
            analysis->has_single = true;
        }
    }
    analysis->quote = quote_from_analysis_(analysis);
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

static void fill_scan_(t_shell_scan *scan, const char *src,
                       const t_shell_analysis *analysis) {
    scan->len = analysis->len;
    scan->quote = analysis->quote;

    if (analysis->quote == '\0') {
        scan->display_len = analysis->len;
        scan->padded_display_len = analysis->len + 1;
    } else if (analysis->quote == '"') {
        scan->display_len = analysis->len + 2;
        scan->padded_display_len = scan->display_len;
    } else {
        t_shell_out escaped = {.dst = NULL, .len = 0};

        shell_escape_bytes_(&escaped, src, analysis->len);
        scan->display_len = escaped.len;
        scan->padded_display_len = escaped.len;
    }
}

static bool needs_raw_quote_(unsigned char c, uint64_t index) {
    const unsigned char cls = g_shell_class_table_[c];

    if (cls & SHELL_CLASS_ALNUM) {
        return false;
    }
    if (cls & SHELL_CLASS_SAFE) {
        return index == 0 && (c == '#' || c == '~');
    }

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
        case '^':
        case '=': return true;
        default: return true;
    }
}
