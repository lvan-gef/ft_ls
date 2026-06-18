#ifndef FT_SHELL_ESCAPE_H
#define FT_SHELL_ESCAPE_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_str.h"

typedef struct s_shell_scan {
    uint64_t len;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
} t_shell_scan;

void shell_scan_str(const t_str *str, t_shell_scan *scan);
uint64_t shell_escaped_len(const t_str *str, char quote, bool pad_unquoted);
bool shell_escape_append(t_str *dst, const t_str *str, char quote,
                         bool pad_unquoted);
t_str *shell_escape_str(const t_str *str, char quote);

#endif // !FT_SHELL_ESCAPE_H
