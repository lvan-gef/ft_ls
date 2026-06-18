#ifndef FT_PRINTER_HELPER_H
#define FT_PRINTER_HELPER_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_str.h"

#ifndef TABSIZE
#define TABSIZE UINT64_C(8)
#endif // ifndef TABSIZE //

#ifndef SPACE_GAP
#define SPACE_GAP UINT64_C(2)
#endif /* ifndef SPACE_GAP */

#ifndef MIN_COLUMN_WIDTH
#define MIN_COLUMN_WIDTH UINT64_C(3)
#endif // ifndef !MIN_COLUMN_WIDTH

#ifndef OUTPUT_BUFFER_CAP
#define OUTPUT_BUFFER_CAP UINT64_C(16384)
#endif // ifndef !OUTPUT_BUFFER_CAP

bool put_mem(t_str *out, const char *src, uint64_t len);
bool put_shell_escaped(t_str *out, const t_str *str, char quote,
                       bool pad_unquoted);
bool flush_str(t_str *out);
bool put_dir_header(t_str *out, const t_str *dir_header);
bool needs_padding(const t_array *context);

#endif // !FT_PRINTER_HELPER_H
