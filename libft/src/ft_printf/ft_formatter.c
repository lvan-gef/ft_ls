/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_formatter.c                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/02/12 17:09:47 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 22:19:27 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ft_printf.h"

ssize_t	to_hex(unsigned long int size, size_t upper, size_t base)
{
	ssize_t	remainder;
	char	letter;
	ssize_t	counter;
	ssize_t	result;

	letter = 'a';
	counter = 0;
	result = 0;
	if (upper)
		letter = 'A';
	if (size > base - 1)
		result = to_hex(size / base, upper, base);
	if (result < 0)
		return (result);
	counter += result;
	remainder = (ssize_t)(size % base);
	if (remainder < 10)
		result = write_ascii((int)remainder + '0');
	else
		result = write_ascii((int)remainder - 10 + letter);
	if (result < 0)
		return (result);
	counter += result;
	return (counter);
}

ssize_t	format_int(int size, size_t upper, size_t base)
{
	ssize_t result;
	ssize_t	counter;
	size_t	new_size;

	result = 0;
	if (size < 0)
	{
		result = write_ascii('-');
		new_size = (unsigned int)(size * -1);
	}
	else
		new_size = (unsigned int)size;
	if (result < 0)
		return (result);
	counter = to_hex(new_size, upper, base);
	if (counter < 0)
		return (counter);
	result += counter;
	return (result);
}

ssize_t	format_ptr(void *ptr, size_t upper, size_t base)
{
	unsigned long int	addr;
	ssize_t				counter;
	ssize_t             result;

	addr = (unsigned long int)ptr;
	result = write_str("0x");
	if (result < 0)
		return (result);
	if (!ptr)
		counter = write_ascii('0');
	else
		counter = to_hex(addr, upper, base);
	if (counter < 0)
		return (counter);
	result += counter;
	return (result);
}
