#ifndef FT_PRINTER_HELPER_H
#define FT_PRINTER_HELPER_H

#include <stdbool.h>

#include "./ft_ls.h"
#include "./ft_out.h"
#include "./ft_str.h"

#ifndef TABSIZE
#define TABSIZE 8
#endif /* ifndef TABSIZE */

#ifndef SPACE_GAP
#define SPACE_GAP 2
#endif /* ifndef SPACE_GAP */

#ifndef MIN_COLUMN_WIDTH
#define MIN_COLUMN_WIDTH 3
#endif /* ifndef MIN_COLUMN_WIDTH */

#ifndef OUTPUT_BUFFER_CAP
#define OUTPUT_BUFFER_CAP 65536
#endif /* ifndef OUTPUT_BUFFER_CAP */

void put_entry_name(t_out *out, const t_entry *entry, bool pad_unquoted,
                    bool color, bool is_stdout);
void put_dir_header(t_out *out, const t_str *dir_header, bool is_stdout);
void put_link_target(t_out *out, const t_str *target, bool is_stdout);

#endif /* ifndef FT_PRINTER_HELPER_H */
