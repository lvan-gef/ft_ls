#include "../../include/libft.h"

void ft_str_to_lower(char *str) {
    size_t index;

    index = 0;
    while (str[index]) {
        if (ft_isalpha(str[index])) {
            str[index] = (char)ft_tolower(str[index]);
        }
        ++index;
    }
}
