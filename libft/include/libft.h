/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   libft.h                                            :+:    :+:            */
/*                                                     +:+                    */
/*   By: lvan-gef <lvan-gef@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2022/10/06 14:39:10 by lvan-gef      #+#    #+#                 */
/*   Updated: 2024/07/07 16:57:05 by lvan-gef      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBFT_H
# define LIBFT_H

# include <stdlib.h>
# include <unistd.h>
# include <limits.h>

# include "./get_next_line.h"
# include "./ft_printf.h"
# include "./ft_fprintf.h"

typedef struct s_list
{
	void			*content;
	struct s_list	*next;
}	t_list;

/**
* @brief Add a node at the end of the linked list
*
* @param lst  Head of the linked list       (t_list**)
* @param node Node to at to the linked list (t_list*)
*
* @return void
*/
void		ft_lstadd_back(t_list **lst, t_list *node);

/**
* @brief Add a node at the beginning of the linked list
*
* @param lst  Head of the linked list       (t_list**)
* @param node Node to at to the linked list (t_list*)
*
* @return void
*/
void		ft_lstadd_front(t_list **lst, t_list *node);

/**
* @brief Clear the linked list
*
* @param lst Head of the linked list  (t_list**)
* @param del Function to clear a node (void (*)(void*))
*
* @return void
*/
void		ft_lstclear(t_list **lst, void (*del)(void*));

/**
* @brief Delete a node from the linked list
*
* @param lst Node to be deleted   (t_list*)
* @param del Function to delete the node (void (*)void*)
*
* @return void
*/
void		ft_lstdelone(t_list *lst, void (*del)(void*));

/**
* @brief Iterates the linked list and call function f for each node
*
* @param lst First node of the linked list (t_list*)
* @param f   Function to call on node      (void (*)(void*))
*
* @return void
*/
void		ft_lstiter(t_list *lst, void (*f)(void *));

/**
* @brief Get the last node of the linked list
*
* @param lst First node of the linked list (t_list*)
*
* @return t_list* | NULL
*/
t_list		*ft_lstlast(t_list *lst);

/**
* @brief It creates a new linked list
* by iterating over each node in the linked list,
* applying the function to the content of the node,
* and creating a new node with the result.
*
* @param lst First node of the linked list (t_list*)
* @param f   Fucntion to call on node      (void* (*)(void*))
* @param del Function to del a node        (void (*)(void*))
*
* @return t_list* | NULL
*/
t_list		*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *));

/**
* @brief Create a new node for a linked list
*
* @param content The data that must be hold in the node (void *)
*
* @return t_list* | NULL
*/
t_list		*ft_lstnew(void *content);

/**
* @brief Get the size if the linked list
*
* @param lst The first node of the linked list
*
* @return int
*/
int			ft_lstsize(t_list *lst);

/**
* @brief Set memory area to zeros
*
* @param s Begin of memory area    (void*)
* @param c Size of the memory area (size_t)
*
* @return void
*/
void		ft_bzero(void *s, size_t c);

/**
* @brief Allocated a memory area and set it to zeros
*
* @param count How many objects you want (size_t)
* @param size  Size of the object        (size_t)
*
* @return void* | NULL
*/
void		*ft_calloc(size_t count, size_t size);

/**
* @brief Find the fist instance of <c> in <s>
*
* @param s Begin of memory area (void*)
* @param c Char to find         (int)
* @param n Size of memory area  (size_t)
*
* @return void* | NULL
*/
void		*ft_memchr(const void *s, int c, size_t n);

/**
* @brief Compare two memory area's for n size to see if they are the same
*
* @param s1 Memory area 1   (void*)
* @param s2 Memory area 2   (void*)
* @param n  Size to compare (size_t)
*
* @return int 0 | 1>
*/
int			ft_memcmp(const void *s1, const void *s2, size_t n);

/**
* @brief Copy <len> bytes over to a memory area
* memory areas does not overlap
*
* @param dst Memory area where copy to   (void*)
* @param src Memory area where copy from (void*)
* @param len Size to be copy             (size_t)
*
* @return void* | NULL
*/
void		*ft_memcpy(void *dst, const void *src, size_t len);

/**
* @brief Copy <len> bytes over to a memory area
* memory areas may overlap
*
* @param dst Memory area where copy to   (void*)
* @param src Memory area where copy from (void*)
* @param len Size to be copy             (size_t)
*
* @return void* | NULL
*/
void		*ft_memmove(void *dst, const void *src, size_t len);

/**
* @brief Set memory area with a value
*
* @param dest Memory area              (void*)
* @param val  Value to set memory area (int)
* @param len  Size of the memory area  (int)
*
* @return void*
*/
void		*ft_memset(void *b, int c, int len);

/**
* @brief Write a char
*
* @param c  Char to write   (char)
* @param fd File descriptor (int)
*
* @return void
*/
void		ft_putchar_fd(char c, int fd);

/**
* @brief Write a string with \n
*
* @param s  String to write (char*)
* @param fd File descriptor (int)
*
* @return void
*/
void		ft_putendl_fd(const char *s, int fd);

/**
* @brief Write a interger
*
* @param n  Interger to write (int)
* @param fd File descriptor   (int)
*
* @return void
*/
void		ft_putnbr_fd(int n, int fd);

/**
* @brief Write a string
*
* @param s  String to write (char *)
* @param fd File descriptor (int)
*
* @return void
*/
void		ft_putstr_fd(const char *s, int fd);

/**
* @brief Convert string to float
*
* @param str The float as string (char*)
*
* @return double
*/
double		ft_atof(const char *str);

/**
* @brief Convert string to int
*
* @param str The int as string (char*)
*
* @return int
*/
int			ft_atoi(const char *str);

/**
* @brief Convert string to long int
*
* @param str The long int as string (char*)
*
* @return long int
*/
long int	ft_atoli(const char *str);

/**
* @brief Check if <c> is alpha or digit
*
* @param c Char to check (int)
*
* @return int 1 | 0
*/
int			ft_isalnum(int c);

/*
* @brief Check if <c> is alpha
*
* @param c Char to check (int)
*
* @return int 1 | 0
*/
int			ft_isalpha(int c);

/**
* @brief Check if <c> is ascii
*
* @param c
*
* @return
*/
int			ft_isascii(int c);

/**
* @brief Check if <c> is digit
*
* @param c Char to check (int)
*
* @return int 1 | 0
*/
int			ft_isdigit(int c);

/**
* @brief Check if <c> is printable
*
* @param c Char to check (int)
*
* @return int 1 | 0
*/
int			ft_isprint(int c);

/**
* @brief Checked if <c> is whitespace
*
* @param c Char to check (int)
*
* @return int 1 | 0
*/
int			ft_iswhitespace(int c);

/**
* @brief Convert interger to string
*
* @param n Interger to convert (int)
*
* @return char* | NULL
*/
char		*ft_itoa(int n);

/**
* @brief Split string given a delim
*
* @param s String to split   (char*)
* @param c Delim to split on (char)
*
* @return char**
*/
char		**ft_split(char const *s, char c);

/**
* @brief Split a string on whitespaces
*
* @param s String to split
*
* @return char** | NULL
*/
char		**ft_split_ws(char const *s);

/**
* @brief Find first instance of <c> in string
*
* @param s String to search in (char*)
* @param c Char to search for  (c)
*
* @return char* | NULL
*/
char		*ft_strchr(const char *s, int c);

/**
* @brief Duplicate string to new memory area
*
* @param s1 String to duplicate (char*)
*
* @return char* | NULL
*/
char		*ft_strdup(const char *s1);

/**
* @brief Applies the function f to each character of <s>
*
* @param s String to modify (char*)
* @param f Function to call (void (*)(unsigned int, char*))
*
* @return void
*/
void		ft_striteri(char *s, void (*f)(unsigned int, char*));

/**
* @brief Join <s1> and <s2> to a new memory area
*
* @param s1 First string to join  (char*)
* @param s2 Second string to join (char*)
*
* @return char* | NULL
*/
char		*ft_strjoin(char const *s1, char const *s2);

/**
* @brief Concat <dst> with <src> for size <dstsize>
*
* @param dst     String to add to (char*)
* @param src     String to append (char*)
* @param dstsize Size to concat   (size_t)
*
* @return size_t
*/
size_t		ft_strlcat(char *dst, const char *src, size_t dstsize);

/**
* @brief Copy <dstsize> bytes over from <src> to <dst>
*
* @param dst     String to copy to   (char*)
* @param src     String to copy from (char *)
* @param dstsize Size to be copy     (size_t)
*
* @return size_t
*/
size_t		ft_strlcpy(char *dst, const char *src, size_t dstsize);

/**
* @brief Get len of string
*
* @param s String to get len of (char*)
*
* @return size_t
*/
size_t		ft_strlen(const char *s);

/**
* @brief Iterates the string and call function <f> for each char
* And return a new memory area
*
* @param s String to be walked           (char*)
* @param f Function to call to each char (char (*)(unsigned int, char))
*
* @return char* | NULL
*/
char		*ft_strmapi(char const *s, char (*f)(unsigned int, char));

/**
* @brief Compare 2 strings to see if they are the same
*
* @param s1 String 1 to compare         (char*)
* @param s2 String 2 to compare         (char*)
* @param n  Size to compare the strings (size_t)
*
* @return int 0 | >0
*/
int			ft_strncmp(const char *s1, const char *s2, size_t n);

/**
* @brief Find needle in a haystack
*
* @param haystack String to search in    (char*)
* @param needle   String to find         (char*)
* @param len      Size if string to find (size_t)
*
* @return
*/
char		*ft_strnstr(const char *haystack, const char *needle, size_t len);

/**
* @brief Find last instance of <c> in string
*
* @param s String to search in (char*)
* @param c Char to search for  (c)
*
* @return char* | NULL
*/
char		*ft_strrchr(const char *s, int c);

/**
* @brief Trim string from a charset
* Create new memory area
*
* @param s1  String to trim            (char*)
* @param set String with chars to trim (char*)
*
* @return char* | NULL
*/
char		*ft_strtrim(char const *s1, char const *set);

/**
* @brief Get a substring from <s> given <start> and <len>
*
* @param s     String to get substring from    (char*)
* @param start Start position of the substring (unsigned int)
* @param len   Len of the substring            (size_t)
*
* @return char* | NULL
*/
char		*ft_substr(char const *s, unsigned int start, size_t len);

/**
* @brief Make lowercase of a char
*
* @param c Char that must be lowercase (int)
*
* @return int c + 32 | c
*/
int			ft_tolower(int c);

/**
* @brief Make uppercase of char
*
* @param c Char that must be uppercase (int)
*
* @return int c - 32 | c
*/
int			ft_toupper(int c);

#endif
