NAME := libft.a

CC        ?= cc
CFLAGS    := -Wall -Wextra -Werror -Wshadow -Wpedantic -Wconversion -Wdouble-promotion
DEBUGFLAG := -g3 -fno-omit-frame-pointer -fsanitize=address,null
DEPSFLAGS := -MMD -MP

SRC_DIR   := src
OBJ_DIR   := obj
INC_DIR   := include

MODULES := ll mem printer str gnl ft_printf ft_fprintf

SRC_DIRS := $(addprefix $(SRC_DIR)/, $(MODULES))
OBJ_DIRS := $(addprefix $(OBJ_DIR)/, $(MODULES))

ll_SRC      := ft_lstnew_bonus.c ft_lstadd_front_bonus.c ft_lstsize_bonus.c \
               ft_lstlast_bonus.c ft_lstadd_back_bonus.c ft_lstdelone_bonus.c \
               ft_lstclear_bonus.c ft_lstiter_bonus.c ft_lstmap_bonus.c

mem_SRC     := ft_bzero.c ft_calloc.c ft_memchr.c ft_memcmp.c ft_memcpy.c \
               ft_memmove.c ft_memset.c

printer_SRC := ft_putchar_fd.c ft_putendl_fd.c ft_putnbr_fd.c ft_putstr_fd.c

str_SRC     := ft_strjoin.c ft_strlcat.c ft_itoa.c ft_atoi.c ft_isdigit.c \
               ft_strlcpy.c ft_strdup.c ft_strncmp.c ft_tolower.c ft_strnstr.c \
               ft_strchr.c ft_striteri.c ft_isascii.c ft_isalnum.c ft_split.c \
               ft_isalpha.c ft_strrchr.c ft_strlen.c ft_isprint.c ft_strtrim.c \
               ft_substr.c ft_strmapi.c ft_toupper.c ft_atoli.c ft_iswhitespace.c \
               ft_split_ws.c ft_atof.c

gnl_SRC     := get_next_line.c get_next_line_utils.c

ft_printf_SRC  := ft_formatter.c ft_printf.c ft_writers.c
ft_fprintf_SRC := ft_fformatter.c ft_fprintf.c ft_fwriters.c

ll_OBJ         := $(ll_SRC:%.c=$(OBJ_DIR)/ll/%.o)
mem_OBJ        := $(mem_SRC:%.c=$(OBJ_DIR)/mem/%.o)
printer_OBJ    := $(printer_SRC:%.c=$(OBJ_DIR)/printer/%.o)
str_OBJ        := $(str_SRC:%.c=$(OBJ_DIR)/str/%.o)
gnl_OBJ        := $(gnl_SRC:%.c=$(OBJ_DIR)/gnl/%.o)
ft_printf_OBJ  := $(ft_printf_SRC:%.c=$(OBJ_DIR)/ft_printf/%.o)
ft_fprintf_OBJ := $(ft_fprintf_SRC:%.c=$(OBJ_DIR)/ft_fprintf/%.o)

OBJECTS := $(ll_OBJ) $(mem_OBJ) $(printer_OBJ) $(str_OBJ) $(gnl_OBJ) $(ft_printf_OBJ) $(ft_fprintf_OBJ)
DEPS    := $(OBJECTS:.o=.d)

HEADERS := -I $(INC_DIR)

LIB := ar rcs

BLUE     := \033[36m
NC       := \033[0m

.PHONY all:
all: $(NAME)  ## Build the library (default)

.PHONY debug:
debug: CFLAGS += $(DEBUGFLAG)  ## Build with debug flags
debug: $(NAME)

.PHONY clean:
clean:  ## Clean object files
	@rm -rf $(OBJ_DIR)
	@echo "Cleaned object files"

.PHONY fclean:
fclean: clean  ## Clean object files and library
	@rm -f $(NAME)
	@echo "Cleaned everything"

.PHONY re:
re: fclean all  ## Clean all and rebuild

.PHONY help:
help:  ## Display this help message
	@echo 'Usage: make $(BLUE)<target>$(NC)'
	@echo 'Available targets:'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  $(BLUE)%-15s$(NC) %s\n", $$1, $$2}' $(MAKEFILE_LIST)

$(NAME): $(OBJECTS)
	$(LIB) $@ $(OBJECTS)
	@echo "Library: '$(NAME)' built successfully"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

$(OBJ_DIRS): | $(OBJ_DIR)
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

-include $(DEPS)
