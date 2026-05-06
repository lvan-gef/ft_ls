NAME   := ft_ls
NAME_D := ft_ls_d
MAKEFLAGS += -j

TERM_SIZE ?= 80

.PHONY: check-term-size
check-term-size:
	@case "$(TERM_SIZE)" in \
		''|*[!0-9]*|0) \
			echo "error: TERM_SIZE must be a positive integer, got '$(TERM_SIZE)'"; \
			exit 1; \
			;; \
	esac

LIBFT_DIR := libft
LIBFT     := $(LIBFT_DIR)/libft.a
LIBFT_D   := $(LIBFT_DIR)/libft_d.a

CC        := cc
CPPFLAGS  := -DTERM_SIZE=$(TERM_SIZE) -I include -I $(LIBFT_DIR)/include
CFLAGS    := -std=c11 -D_DEFAULT_SOURCE                                        \
			 -Wall -Wextra -Werror -Wshadow -Wpedantic                         \
			 -Wconversion -Wsign-conversion -Wdouble-promotion                 \
			 -Wformat=2 -Wformat-security                                      \
			 -Wnull-dereference -Wcast-align -Wswitch-enum -Wundef             \
			 -Wstrict-prototypes -Wmissing-prototypes                          \
			 -Wredundant-decls -Wwrite-strings                                 \
			 -Wimplicit-fallthrough                                            \
			 -Wcast-qual                                                       \
			 -Wvla -Walloca -Wold-style-definition

DEPSFLAGS := -MMD -MP

# Release flags (with hardening)
R_CFLAGS  := -DNDEBUG -O3 -march=native -fomit-frame-pointer -fPIE -fstack-clash-protection
R_LDFLAGS := -pie -Wl,-z,relro,-z,now

# Debug flags
SANITIZERS := -fsanitize=address,undefined,null,leak,integer-divide-by-zero,signed-integer-overflow
# SANITIZERS :=
D_CFLAGS   := -g3 -fno-omit-frame-pointer -fstack-protector-strong $(SANITIZERS)
D_LDFLAGS  := $(SANITIZERS) -rdynamic

SRC_DIR := src

SRC_FILES := ft_arena.c ft_array.c ft_entry.c ft_free_list.c ft_helper.c       \
			 ft_parse.c ft_printer.c ft_printer_helper.c ft_print_list.c       \
			 ft_shell_escape.c ft_sort.c ft_str.c ft_walk.c main.c

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


# Build rules
.PHONY: all
all: check-term-size $(NAME)  ## Build release version (default)

.PHONY: debug
debug: check-term-size $(NAME_D)  ## Build debug version with ASAN

.PHONY: tester
tester:  ## run the tester
	python3 ./tester/main.py -d

.PHONY: clean
clean:  ## Clean object files
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -rf $(R_OBJ_DIR) $(D_OBJ_DIR)

.PHONY: fclean
fclean:  ## Clean object, bin
	@$(MAKE) clean
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME) $(NAME_D)

.PHONY: re
re:  ## Clean all and recompile
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: re-debug
re-debug:  ## Clean all and rebuild debug
	@$(MAKE) fclean
	@$(MAKE) debug

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
$(R_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | check-term-size $(R_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) -c $< -o $@

# Debug pattern rule
$(D_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | check-term-size $(D_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_CFLAGS) $(DEPSFLAGS) -c $< -o $@

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR)

$(LIBFT_D):
	@$(MAKE) -C $(LIBFT_DIR) debug

$(R_OBJ_DIR):
	@mkdir -p $@

$(D_OBJ_DIR):
	@mkdir -p $@

-include $(R_DEPS) $(D_DEPS)
