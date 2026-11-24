#ifndef FT_PRINTER_HELPER_H
#define FT_PRINTER_HELPER_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_ls.h"
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

bool put_mem(t_str *out, const char *src, size_t len);
bool put_mem_fd(t_str *out, const char *src, size_t len, int fd);
bool put_entry_name(t_str *out, const t_entry *entry, bool pad_unquoted,
                    bool color, bool is_stdout);
bool flush_fd(t_str *out, int fd);
bool put_dir_header(t_str *out, const t_str *dir_header, bool is_stdout);

#endif /* ifndef FT_PRINTER_HELPER_H */
