#ifndef FT_HELPER_H
#define FT_HELPER_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_free_list.h"
#include "./ft_str.h"

typedef bool (*ft_write_mem_fn)(void *ctx, const char *src, uint64_t len);

uint64_t len_of_nbr(uint64_t nbr);
char shell_quote_style(const t_str *str);
bool has_shell_quote_char(const t_str *str);
uint64_t shell_display_len(const t_str *str, char quote, bool pad_unquoted);
bool write_shell_escaped(void *ctx, ft_write_mem_fn writer, const t_str *str,
                         char quote, bool pad_unquoted);
t_str *shell_escape_str(free_list *fl, const t_str *str, char quote);

#endif // !FT_HELPER_H
