/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_lstadd_front_bonus.c                            :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/12/01 20:51:22 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:07:57 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/libft.h"

void	ft_lstadd_front(t_list **lst, t_list *node)
{
	if (!node)
		return ;
	if (*lst == NULL)
		*lst = node;
	else
	{
		node->next = *lst;
		*lst = node;
	}
}
