#ifndef FT_SHELL_ESCAPE_H
#define FT_SHELL_ESCAPE_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_str.h"

#include "./ft_ls.h"

void shell_scan_str(const t_str *str, t_shell_scan *scan);
uint64_t shell_escaped_len(const t_str *str, char quote, bool pad_unquoted);
t_str *shell_escape_str(const t_str *str, char quote);
bool shell_escape_append_len(t_str *dst, const t_str *str, char quote,
                             bool pad_unquoted, uint64_t escaped_len);

#endif /* ifndef FT_SHELL_ESCAPE_H */
