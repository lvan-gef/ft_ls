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

#include "../../include/ft_fprintf.h"

ssize_t	f_to_hex(unsigned long int size, size_t upper, size_t base, int fd)
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
		result = f_to_hex(size / base, upper, base, fd);
	if (result < 0)
		return (result);
	counter += result;
	remainder = (ssize_t)(size % base);
	if (remainder < 10)
		result = f_write_ascii((int)remainder + '0', fd);
	else
		result = f_write_ascii((int)remainder - 10 + letter, fd);
	if (result < 0)
		return (result);
	counter += result;
	return (counter);
}

ssize_t	f_format_int(int size, size_t upper, size_t base, int fd)
{
	ssize_t result;
	ssize_t	counter;
	size_t	new_size;

	result = 0;
	if (size < 0)
	{
		result = f_write_ascii('-', fd);
		new_size = (unsigned int)(size * -1);
	}
	else
		new_size = (unsigned int)size;
	if (result < 0)
		return (result);
	counter = f_to_hex(new_size, upper, base, fd);
	if (counter < 0)
		return (counter);
	result += counter;
	return (result);
}

ssize_t	f_format_ptr(void *ptr, size_t upper, size_t base, int fd)
{
	unsigned long int	addr;
	ssize_t				counter;
	ssize_t             result;

	addr = (unsigned long int)ptr;
	result = f_write_str("0x", fd);
	if (result < 0)
		return (result);
	if (!ptr)
		counter = f_write_ascii('0', fd);
	else
		counter = f_to_hex(addr, upper, base, fd);
	if (counter < 0)
		return (counter);
	result += counter;
	return (result);
}
