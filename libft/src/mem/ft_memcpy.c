/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_memcpy.c                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 16:38:50 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:27:34 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void	*ft_memcpy(void *dst, const void *src, size_t len)
{
	char		*dst_;
	const char	*src_;

	if (!src || !dst)
		return (NULL);
	dst_ = dst;
	src_ = src;
	while (len--)
		*dst_++ = *src_++;
	return (dst);
}
