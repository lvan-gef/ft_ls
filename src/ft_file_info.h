#ifndef FT_FILE_INFO_H
#define FT_FILE_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./ft_str.h"
#include "./ft_printer.h"

#ifndef PERMISSION_SIZE
#define PERMISSION_SIZE 12
#endif /* ifndef PERMISSION_SIZE */

#ifndef DT_LEN
#define DT_LEN 13
#endif /* ifndef DT_LEN */

typedef struct s_file_info {
    char perm[PERMISSION_SIZE];
    size_t perm_len;
    char dt[DT_LEN];
    t_str *username;
    t_str *groupname;
    t_str *symlink;
} t_file_info;

bool prepare_list_infos(const t_print_request *req, t_file_info **infos);

#endif /* ifndef FT_FILE_INFO_H */
