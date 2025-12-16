/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_strnstr.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/18 12:54:18 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/05/05 00:39:14 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t	index;
	size_t	li_index;
	size_t	check_point;

	index = 0;
	li_index = 0;
	if (!ft_strlen(needle))
		return ((char *)haystack);
	if (!haystack)
		return (NULL);
	while (index < len && haystack[index] != '\0')
	{
		check_point = index;
		while (index < len && haystack[index] == needle[li_index])
		{
			li_index++;
			index++;
			if (needle[li_index] == '\0')
				return ((char *)&haystack[check_point]);
		}
		index = check_point + 1;
		li_index = 0;
	}
	return (NULL);
}
