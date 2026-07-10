#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>
#include <stdint.h>

#include "../include/ft_array.h"
#include "../include/ft_str.h"

#include "./ft_arena.h"

#define DIRECTORY "\033[01;34m"
#define SYMLINK "\033[01;36m"
#define SOCKET "\033[01;35m"
#define FIFO "\033[33m"
#define EXECUTABLE "\033[01;32m"
#define BLOCKCHAR "\033[01;33m"
#define RESET "\033[0m"

typedef struct s_print_request {
    const t_array *entries;
    const t_array *list_width_context;
    const t_str *dir_header;
    t_str *buffer;
    t_arena *arena;
    bool quote_padding;
    bool list_mode;
    bool print_total;
    bool no_owner;
    bool no_group;
    bool access_time;
    bool color;
    bool is_stdout;
    uint64_t term_size;
} t_print_request;

bool printer(const t_print_request *req);
bool printer_list(const t_print_request *req);
uint64_t get_terminal_width(void);

#endif /* ifndef FT_PRINTER_H */
