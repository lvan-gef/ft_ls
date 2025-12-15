NAME := ft_ls

CC        := cc
CFLAGS    := -std=c11 -Wall -Wextra -Werror -Wshadow -Wpedantic -Wconversion -Wdouble-promotion
DEBUGFLAG := -g3 -fno-omit-frame-pointer -fsanitize=address,null
DEPSFLAGS := -MMD -MP

SRC_DIR   := src
OBJ_DIR   := obj
DEP_DIR   := $(OBJ_DIR)

SRC_FILES   := main.c parser.c

SRCS := $(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJECTS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)


BLUE := \033[36m
MARGENTA := \033[35m
NC := \033[0m

LIBFT_DIR  := libft
LIBFT      := $(LIBFT_DIR)/libft.a

HEADERS := -I include -I $(LIBFT_DIR)/include

BUILD_MODE ?= all

.PHONY: debug
debug: $(NAME)  ## Build the debug version with ASAN
debug: BUILD_MODE := debug
debug: CFLAGS += $(DEBUGFLAG)

# Build rules
.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: clean
clean:  ## Clean object files
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -rf $(OBJ_DIR)

.PHONY: fclean
fclean: clean  ## Clean object, bin
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME)

.PHONY: re
re: fclean all  ## Clean all and recompile

.PHONY: fmt
fmt:  ## Format code via clang-format
	@echo "Format code"
	@find . -type f -name "*.c" -print0 | xargs -0 clang-format -i
	@find . -type f -name "*.h" -print0 | xargs -0 clang-format -i

.PHONY: help
help:  ## Get help
	@echo -e 'Usage: make ${BLUE}<target>${NC}'
	@echo -e 'Available targets:'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  ${BLUE}%-15s${NC} %s\n", $$1, $$2}' $(MAKEFILE_LIST)

$(NAME): $(LIBFT) $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBFT) -o $@
	@echo "Build complete: $(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR) $(BUILD_MODE)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

-include $(DEPS)
