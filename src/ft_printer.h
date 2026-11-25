#ifndef FT_PRINTER_H
#define FT_PRINTER_H

#include <stdbool.h>
#include <stdint.h>

#include "./ft_arena.h"
#include "./ft_array.h"
#include "./ft_out.h"
#include "./ft_parse.h"
#include "./ft_str.h"

#define DIRECTORY "\033[01;34m"
#define SYMLINK "\033[01;36m"
#define SOCKET "\033[01;35m"
#define FIFO "\033[33m"
#define EXECUTABLE "\033[01;32m"
#define BLOCKCHAR "\033[01;33m"
#define RESET "\033[0m"

typedef struct s_print_ctx {
    const t_args *args;
    t_out *out;
    t_arena *arena;
    bool is_stdout;
    size_t term_size;
} t_print_ctx;

typedef struct s_print_request {
    const t_print_ctx *ctx;
    const t_array *entries;
    const t_array *list_width_context;
    const t_str *dir_header;
    bool quoted;
    bool print_total;
} t_print_request;

bool printer(const t_print_request *req);
bool printer_list(const t_print_request *req);
size_t get_terminal_width(void);

#endif /* ifndef FT_PRINTER_H */
