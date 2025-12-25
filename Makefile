NAME   := ft_ls
NAME_D := ft_ls_d

CC        := cc
CFLAGS    := -Wall -Wextra -Werror -Wshadow -Wpedantic -Wconversion -Wdouble-promotion
DEPSFLAGS := -MMD -MP

# Release flags
R_CFLAGS  := -DNDEBUG -O3 -march=native -fomit-frame-pointer
R_LDFLAGS :=

# Debug flags
D_CFLAGS  := -g3 -fno-omit-frame-pointer -fsanitize=address,undefined,null,leak,integer-divide-by-zero,signed-integer-overflow
D_LDFLAGS := -fsanitize=address,undefined

SRC_DIR := src

SRC_FILES := main.c ft_parser.c ft_assert.c ft_walk.c ft_print.c ft_sort.c ft_array.c ft_free.c

SRCS := $(addprefix $(SRC_DIR)/, $(SRC_FILES))

# Release objects
R_OBJ_DIR := obj
R_OBJECTS := $(SRCS:$(SRC_DIR)/%.c=$(R_OBJ_DIR)/%.o)
R_DEPS    := $(R_OBJECTS:.o=.d)

# Debug objects
D_OBJ_DIR := obj_debug
D_OBJECTS := $(SRCS:$(SRC_DIR)/%.c=$(D_OBJ_DIR)/%.o)
D_DEPS    := $(D_OBJECTS:.o=.d)

BLUE := \033[36m
MARGENTA := \033[35m
NC := \033[0m

LIBFT_DIR := libft
LIBFT     := $(LIBFT_DIR)/libft.a
LIBFT_D   := $(LIBFT_DIR)/libft_d.a

HEADERS := -I include -I $(LIBFT_DIR)/include

# Build rules
.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: debug
debug: $(NAME_D)  ## Build debug version with ASAN

.PHONY: tester
tester:  ## run the tester
	python3 ./tester/main.py

.PHONY: clean
clean:  ## Clean object files
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -rf $(R_OBJ_DIR) $(D_OBJ_DIR)

.PHONY: fclean
fclean: clean  ## Clean object, bin
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME) $(NAME_D)

.PHONY: re
re: fclean all  ## Clean all and recompile

.PHONY: re-debug
re-debug: fclean debug  ## Clean all and recompile debug

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

# Release build
$(NAME): $(LIBFT) $(R_OBJECTS)
	$(CC) $(R_OBJECTS) $(R_LDFLAGS) -s $(LIBFT) -o $@
	@echo "Build complete: $(NAME) (release)"

# Debug build
$(NAME_D): $(LIBFT_D) $(D_OBJECTS)
	$(CC) $(D_OBJECTS) $(D_LDFLAGS) $(LIBFT_D) -o $@
	@echo "Build complete: $(NAME_D) (debug)"

# Release pattern rule
$(R_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(R_OBJ_DIR)
	$(CC) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

# Debug pattern rule
$(D_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(D_OBJ_DIR)
	$(CC) $(CFLAGS) $(D_CFLAGS) $(DEPSFLAGS) $(HEADERS) -c $< -o $@

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR)

$(LIBFT_D):
	@$(MAKE) -C $(LIBFT_DIR) debug

$(R_OBJ_DIR):
	@mkdir -p $@

$(D_OBJ_DIR):
	@mkdir -p $@

-include $(R_DEPS) $(D_DEPS)
