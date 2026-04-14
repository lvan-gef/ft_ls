#include <stdint.h>

#include "../include/ft_assert.h"
#include "../include/ft_free_list.h"
#include "../include/ft_helper.h"
#include "../include/ft_str.h"

#include "../libft/include/libft.h"

uint64_t len_of_nbr(uint64_t nbr) {
    uint64_t len = 1;

    while (nbr >= 10) {
        nbr /= 10;
        ++len;
    }

    return len;
}
