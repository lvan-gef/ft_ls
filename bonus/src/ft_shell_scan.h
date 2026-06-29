#ifndef FT_SHELL_SCAN_H
#define FT_SHELL_SCAN_H

#include <stdint.h>

typedef struct s_shell_scan {
    uint64_t len;
    uint64_t display_len;
    uint64_t padded_display_len;
    char quote;
} t_shell_scan;

#endif /* ifndef FT_SHELL_SCAN_H */
