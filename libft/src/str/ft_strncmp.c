/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strncmp.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/18 13:00:15 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:46:21 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

int	ft_strncmp(const char *s1, const char *s2, size_t n)
{
	size_t				index;
	const unsigned char	*s1_;
	const unsigned char	*s2_;

	s1_ = (unsigned char *)s1;
	s2_ = (unsigned char *)s2;
	index = 0;
	while (index < n)
	{
		if (s1_[index] == '\0' && s2_[index] == '\0')
			break ;
		if (s1_[index] != s2_[index])
			return (s1_[index] - s2_[index]);
		index++;
	}
	return (0);
}
