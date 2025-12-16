/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_lstadd_back_bonus.c                             :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/12/01 22:43:12 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:07:45 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void	ft_lstadd_back(t_list **lst, t_list *node)
{
	t_list	*cur;

	if (!node)
		return ;
	cur = NULL;
	if (*lst == NULL)
		*lst = node;
	else
	{
		cur = *lst;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = node;
	}
}
