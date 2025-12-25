NAME   := libft.a
NAME_D := libft_d.a

CC        ?= cc
CFLAGS    := -Wall -Wextra -Werror -Wshadow -Wpedantic -Wconversion -Wdouble-promotion
DEPSFLAGS := -MMD -MP

# Release flags
R_CFLAGS  := -DNDEBUG -O3 -march=native -fomit-frame-pointer

# Debug flags
D_CFLAGS  := -g3 -fno-omit-frame-pointer -fsanitize=address,undefined,null,leak,integer-divide-by-zero,signed-integer-overflow

SRC_DIR   := src
INC_DIR   := include

MODULES := ll mem printer str gnl ft_printf ft_fprintf

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

# Release objects
R_OBJ_DIR := obj
R_OBJ_DIRS := $(addprefix $(R_OBJ_DIR)/, $(MODULES))
R_OBJECTS := $(ll_SRC:%.c=$(R_OBJ_DIR)/ll/%.o) \
             $(mem_SRC:%.c=$(R_OBJ_DIR)/mem/%.o) \
             $(printer_SRC:%.c=$(R_OBJ_DIR)/printer/%.o) \
             $(str_SRC:%.c=$(R_OBJ_DIR)/str/%.o) \
             $(gnl_SRC:%.c=$(R_OBJ_DIR)/gnl/%.o) \
             $(ft_printf_SRC:%.c=$(R_OBJ_DIR)/ft_printf/%.o) \
             $(ft_fprintf_SRC:%.c=$(R_OBJ_DIR)/ft_fprintf/%.o)
R_DEPS := $(R_OBJECTS:.o=.d)

# Debug objects
D_OBJ_DIR := obj_debug
D_OBJ_DIRS := $(addprefix $(D_OBJ_DIR)/, $(MODULES))
D_OBJECTS := $(ll_SRC:%.c=$(D_OBJ_DIR)/ll/%.o) \
             $(mem_SRC:%.c=$(D_OBJ_DIR)/mem/%.o) \
             $(printer_SRC:%.c=$(D_OBJ_DIR)/printer/%.o) \
             $(str_SRC:%.c=$(D_OBJ_DIR)/str/%.o) \
             $(gnl_SRC:%.c=$(D_OBJ_DIR)/gnl/%.o) \
             $(ft_printf_SRC:%.c=$(D_OBJ_DIR)/ft_printf/%.o) \
             $(ft_fprintf_SRC:%.c=$(D_OBJ_DIR)/ft_fprintf/%.o)
D_DEPS := $(D_OBJECTS:.o=.d)

HEADERS := -I $(INC_DIR)

LIB := ar rcs

BLUE     := \033[36m
NC       := \033[0m

.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: debug
debug: $(NAME_D)  ## Build debug version

.PHONY: clean
clean:  ## Clean object files
	@rm -rf $(R_OBJ_DIR) $(D_OBJ_DIR)
	@echo "Cleaned object files"

.PHONY: fclean
fclean: clean  ## Clean object files and library
	@rm -f $(NAME) $(NAME_D)
	@echo "Cleaned everything"

.PHONY: re
re: fclean all  ## Clean all and rebuild

.PHONY: re-debug
re-debug: fclean debug  ## Clean all and rebuild debug

.PHONY: help
help:  ## Display this help message
	@echo 'Usage: make $(BLUE)<target>$(NC)'
	@echo 'Available targets:'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  $(BLUE)%-15s$(NC) %s\n", $$1, $$2}' $(MAKEFILE_LIST)

# Release build
$(NAME): $(R_OBJECTS)
	$(LIB) $@ $(R_OBJECTS)
	@echo "Library: '$(NAME)' built successfully (release)"

# Debug build
$(NAME_D): $(D_OBJECTS)
	$(LIB) $@ $(D_OBJECTS)
	@echo "Library: '$(NAME_D)' built successfully (debug)"

# Release pattern rule
$(R_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(R_OBJ_DIRS)
	$(CC) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

# Debug pattern rule
$(D_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(D_OBJ_DIRS)
	$(CC) $(CFLAGS) $(D_CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

$(R_OBJ_DIRS): | $(R_OBJ_DIR)
	@mkdir -p $@

$(D_OBJ_DIRS): | $(D_OBJ_DIR)
	@mkdir -p $@

$(R_OBJ_DIR):
	@mkdir -p $@

$(D_OBJ_DIR):
	@mkdir -p $@

-include $(R_DEPS) $(D_DEPS)
