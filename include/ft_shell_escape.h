#ifndef FT_SHELL_ESCAPE_H
#define FT_SHELL_ESCAPE_H

#include <stdbool.h>

#include "./ft_free_list.h"
#include "./ft_str.h"

typedef struct s_shell_scan {
    uint64_t len;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
} t_shell_scan;

bool escaped_out(t_str *dst, const t_str *str, char quote, bool pad_unquoted);
t_str *shell_escape_str(free_list *fl, const t_str *str, char quote);
void shell_scan_str(const t_str *str, t_shell_scan *scan);
void shell_scan_cstr(const char *src, t_shell_scan *scan);

#endif // !FT_SHELL_ESCAPE_H
