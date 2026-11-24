#ifndef FT_SHELL_ESCAPE_H
#define FT_SHELL_ESCAPE_H

#include <stdbool.h>

#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_str.h"

void shell_scan_str(const t_str *str, t_shell_scan *scan);
void shell_escape_put(t_out *out, const t_str *str, char quote,
                      bool pad_unquoted);

#endif /* ifndef FT_SHELL_ESCAPE_H */
