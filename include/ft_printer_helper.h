#ifndef FT_PRINTER_HELPER_H
#define FT_PRINTER_HELPER_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_array.h"
#include "./ft_str.h"

#ifndef TABSIZE
#define TABSIZE UINT64_C(8)
#endif // ifndef TABSIZE //

#ifndef SPACE_GAP
#define SPACE_GAP UINT64_C(2)
#endif /* ifndef SPACE_GAP */

#ifndef MIN_COLUMN_WIDTH
#define MIN_COLUMN_WIDTH UINT64_C(3)
#endif // !MIN_COLUMN_WIDTH

#ifndef OUTPUT_BUFFER_CAP
#define OUTPUT_BUFFER_CAP UINT64_C(4096)
#endif // !OUTPUT_BUFFER_CAP

bool put_mem(t_str *out, const char *src, uint64_t len);
bool flush_str(t_str *out);
bool have_quotes(t_array *array);

#endif // !FT_PRINTER_HELPER_H
