#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_shell_escape.h"
#include "./ft_str.h"
#include "./ft_utils.h"

typedef struct s_shell_out {
    t_out *dst;
    size_t len;
} t_shell_out;

typedef enum e_quote_mode {
    MODE_NONE,
    MODE_SINGLE,
    MODE_ANSI,
} t_quote_mode;

static void emit_bytes_(t_shell_out *out, const char *src, size_t len);
static void emit_ansi_byte_(t_shell_out *out, unsigned char byte);
static void shell_escape_bytes_(t_shell_out *out, const char *src, size_t len);
static void escape_byte_(t_shell_out *out, t_quote_mode *mode, unsigned char c);
static size_t escaped_len_(const char *src, size_t len, char quote);
static bool needs_raw_quote_(unsigned char c, size_t index);
static void put_literal_(t_out *out, const char *src, size_t len);
static void switch_mode_(t_shell_out *out, t_quote_mode *mode,
                         const t_quote_mode next);

void shell_scan_str(const t_str *str, t_shell_scan *scan) {
    bool needs_quote = false;
    bool has_single = false;
    bool can_use_double = true;

    for (size_t index = 0; index < str->len; ++index) {
        const unsigned char c = (unsigned char)str->str[index];

        if (c == '\'') {
            has_single = true;
        }

        if (needs_raw_quote_(c, index)) {
            needs_quote = true;
            if (c != ' ' && c != '\'') {
                can_use_double = false;
            }
        }
    }

    if (!needs_quote) {
        scan->quote = '\0';
    } else if (has_single && can_use_double) {
        scan->quote = '"';
    } else {
        scan->quote = '\'';
    }

    scan->display_len = escaped_len_(str->str, str->len, scan->quote);
}

void shell_escape_put(t_out *out, const t_str *str, const char quote,
                      const bool pad_unquoted) {
    if (!quote) {
        if (pad_unquoted) {
            out_put(out, " ", 1);
        }

        put_literal_(out, str->str, str->len);
        return;
    }

    if (quote == '"') {
        out_put(out, "\"", 1);
        out_put(out, str->str, str->len);
        out_put(out, "\"", 1);
        return;
    }

    shell_escape_bytes_(&(t_shell_out){.dst = out, .len = 0}, str->str,
                        str->len);
}

static void emit_bytes_(t_shell_out *out, const char *src, const size_t len) {
    if (out->dst) {
        out_put(out->dst, src, len);
    }

    out->len += len;
}

static void emit_ansi_byte_(t_shell_out *out, const unsigned char byte) {
    char repr[4];
    size_t len = 2;

    repr[0] = '\\';
    switch (byte) {
        case '\a': repr[1] = 'a'; break;
        case '\b': repr[1] = 'b'; break;
        case '\t': repr[1] = 't'; break;
        case '\n': repr[1] = 'n'; break;
        case '\v': repr[1] = 'v'; break;
        case '\f': repr[1] = 'f'; break;
        case '\r': repr[1] = 'r'; break;
        default:
            len = 4;
            repr[1] = (char)('0' + byte / 64);
            repr[2] = (char)('0' + byte / 8 % 8);
            repr[3] = (char)('0' + byte % 8);
            break;
    }

    emit_bytes_(out, repr, len);
}

static void shell_escape_bytes_(t_shell_out *out, const char *src,
                                const size_t len) {
    t_quote_mode mode = MODE_NONE;

    switch_mode_(out, &mode, MODE_SINGLE);
    for (size_t i = 0; i < len; ++i) {
        escape_byte_(out, &mode, (unsigned char)src[i]);
    }

    switch_mode_(out, &mode, MODE_NONE);
}

static void escape_byte_(t_shell_out *out, t_quote_mode *mode,
                         const unsigned char c) {
    if (ft_isprint(c) && c != '\'') {
        switch_mode_(out, mode, MODE_SINGLE);
        emit_bytes_(out, (const char *)&c, 1);
        return;
    }

    if (c == '\'') {
        if (*mode == MODE_SINGLE) {
            emit_bytes_(out, "'\\''", 4);
        } else {
            switch_mode_(out, mode, MODE_NONE);
            emit_bytes_(out, "\\'", 2);
        }

        return;
    }

    switch_mode_(out, mode, MODE_ANSI);
    emit_ansi_byte_(out, c);
}

static size_t escaped_len_(const char *src, const size_t len,
                           const char quote) {
    if (!quote) {
        return len;
    }

    if (quote == '"') {
        return len + 2;
    }

    t_shell_out out = {.dst = NULL, .len = 0};

    shell_escape_bytes_(&out, src, len);
    return out.len;
}

static bool needs_raw_quote_(const unsigned char c, const size_t index) {
    if (ft_isalpha(c) || ft_isdigit(c) || ft_memchr("_-./+:,@%{}", c, 11)) {
        return false;
    }

    if (c == '#' || c == '~') {
        return index == 0;
    }

    return true;
}

static void put_literal_(t_out *out, const char *src, const size_t len) {
    size_t start = 0;

    for (size_t i = 0; i < len; ++i) {
        if (src[i] == '\n') {
            out_put(out, src + start, i - start);
            out_put(out, "?", 1);
            start = i + 1;
        }
    }

    out_put(out, src + start, len - start);
}

static void switch_mode_(t_shell_out *out, t_quote_mode *mode,
                         const t_quote_mode next) {
    if (*mode == next) {
        return;
    }

    if (*mode != MODE_NONE) {
        emit_bytes_(out, "'", 1);
    }

    if (next == MODE_SINGLE) {
        emit_bytes_(out, "'", 1);
    } else if (next == MODE_ANSI) {
        emit_bytes_(out, "$'", 2);
    }

    *mode = next;
}
