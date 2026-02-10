#include "../include/ft_printer.h"
#include "../include/ft_array.h"
#include "../include/ft_assert.h"
#include "../include/ft_parse.h"
#include "../include/ft_path.h"
#include "../include/ft_sort.h"

#include "../libft/include/ft_fprintf.h"

void printer(t_args *args, t_array *files) {
    ASSERT_NOTNULL(args);
    ASSERT_NOTNULL(files);
    ASSERT_GT(files->len, 0);
    ASSERT_NOTNULL(files->data[0]);

    sort(files, args->reverse, args->time);
    size_t index = 0;
    while (index < files->len - 1) {
        t_entry *entry = files->data[index];
        ft_fprintf(STDOUT_FILENO, "%s  ", entry->name->str);
        ++index;
    }

    t_entry *entry = files->data[index];
    ft_fprintf(STDOUT_FILENO, "%s\n", entry->name->str);
}
