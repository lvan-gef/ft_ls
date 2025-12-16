/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_putnbr_fd.c                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/27 17:00:47 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/11/25 21:59:51 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void	ft_putnbr_fd(int n, int fd)
{
	if (n == -2147483648)
		ft_putstr_fd((char *)INT_MIN, fd);
	else if (n > 9 && n <= INT_MAX)
	{
		ft_putnbr_fd(n / 10, fd);
		ft_putnbr_fd(n % 10, fd);
	}
	else if (n < 0 && n > INT_MIN)
	{
		n = -n;
		ft_putchar_fd('-', fd);
		ft_putnbr_fd(n, fd);
	}
	else
		ft_putchar_fd((char)(n + '0'), fd);
}
