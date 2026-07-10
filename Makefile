NAME            := ft_ls
NAME_D          := ft_ls_d
NAME_B          := ft_ls_bonus
NAME_B_D        := ft_ls_bonus_d
MAKEFLAGS       += -j

TERM_SIZE       ?= 80

ifeq ($(shell test "$(TERM_SIZE)" -gt 0 2>/dev/null && printf valid || printf invalid),invalid)
$(error TERM_SIZE must be a positive integer, got '$(TERM_SIZE)')
endif

CC              ?= cc
ANALYZER_CC     ?= gcc

CPPFLAGS        := -DTERM_SIZE=$(TERM_SIZE) -I include
B_CPPFLAGS      := -I bonus/include -I bonus
CFLAGS          := -std=c11 -D_DEFAULT_SOURCE                                  \
			       -Wall -Wextra -Werror -Wshadow -Wpedantic                   \
			       -Wconversion -Wsign-conversion                              \
			       -Wformat=2 -Wformat-security                                \
			       -Wnull-dereference -Wcast-align -Wswitch-enum -Wundef       \
			       -Wstrict-prototypes -Wmissing-prototypes                    \
			       -Wredundant-decls -Wwrite-strings                           \
			       -Wimplicit-fallthrough -Wcast-qual                          \
			       -Wvla -Walloca -Wold-style-definition                       \
			       -Wbad-function-cast -Wmissing-declarations                  \
                   -Wstrict-overflow=5 -Wdate-time -Wframe-larger-than=4096

DEPSFLAGS       := -MMD -MP

R_CFLAGS        := -DNDEBUG -O3 -march=native -fomit-frame-pointer             \
			       -fPIE -fstack-clash-protection -D_FORTIFY_SOURCE=3          \
			       -fstack-protector-strong
R_LDFLAGS       := -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

SANITIZERS      := -fsanitize=address,undefined,null,leak,integer-divide-by-zero,signed-integer-overflow,bounds,pointer-compare,pointer-subtract
D_CFLAGS        := -g3 -fno-omit-frame-pointer -fstack-protector-strong $(SANITIZERS)
D_LDFLAGS       := $(SANITIZERS) -rdynamic

SRC_DIR         := src
BONUS_SRC_DIR   := bonus/src
SRC_FILES       := ft_arena.c ft_array.c ft_file_info.c ft_parse.c ft_printer.c\
                   ft_printer_helper.c ft_printer_list.c ft_shell_escape.c     \
			       ft_sort.c ft_str.c ft_str_arena.c ft_symlink.c ft_utils.c   \
			       ft_walk.c ft_walk_entry.c main.c

SRCS            := $(addprefix $(SRC_DIR)/, $(SRC_FILES))
B_SRCS          := $(addprefix $(BONUS_SRC_DIR)/, $(SRC_FILES))

R_OBJ_DIR       := obj
R_OBJECTS       := $(SRCS:$(SRC_DIR)/%.c=$(R_OBJ_DIR)/%.o)
R_DEPS          := $(R_OBJECTS:.o=.d)

D_OBJ_DIR       := obj_debug
D_OBJECTS       := $(SRCS:$(SRC_DIR)/%.c=$(D_OBJ_DIR)/%.o)
D_DEPS          := $(D_OBJECTS:.o=.d)

B_OBJ_DIR       := obj_bonus
B_OBJECTS       := $(B_SRCS:$(BONUS_SRC_DIR)/%.c=$(B_OBJ_DIR)/%.o)
B_DEPS          := $(B_OBJECTS:.o=.d)

BD_OBJ_DIR      := obj_bonus_debug
BD_OBJECTS      := $(B_SRCS:$(BONUS_SRC_DIR)/%.c=$(BD_OBJ_DIR)/%.o)
BD_DEPS         := $(BD_OBJECTS:.o=.d)

BLUE            := \033[36m
MARGENTA        := \033[35m
NC              := \033[0m

TESTER          := python3 ./tester/main.py
TEST_COLS_START ?= 80
TEST_COLS_END   ?= 81
TEST_COLS_STEP  ?= 1
BONUS_FLAGS     ?= g,u,f,d,o

TESTER_COLS     := --cols-start $(TEST_COLS_START)                             \
			       --cols-end $(TEST_COLS_END)                                 \
			       --cols-step $(TEST_COLS_STEP)

.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: debug
debug: $(NAME_D)  ## Build debug version with ASAN

.PHONY: bonus
bonus: $(NAME_B)  ## Build bonus release version

.PHONY: bonus-debug
bonus-debug: $(NAME_B_D)  ## Build bonus debug version with ASAN

.PHONY: tester
tester:  ## Run mandatory tester against debug binary
	$(TESTER) --bin ./$(NAME_D) $(TESTER_COLS)

.PHONY: tester-release
tester-release:  ## Run mandatory tester against release binary
	$(TESTER) --bin ./$(NAME) $(TESTER_COLS)

.PHONY: tester-bonus
tester-bonus: $(NAME_B_D)  ## Run bonus tester against debug binary
	$(TESTER) --bin ./$(NAME_B_D) --bonus-flags $(BONUS_FLAGS) $(TESTER_COLS)

.PHONY: tester-bonus-release
tester-bonus-release: $(NAME_B)  ## Run bonus tester against release binary
	$(TESTER) --bin ./$(NAME_B) --bonus-flags $(BONUS_FLAGS) $(TESTER_COLS)

.PHONY: analyze
analyze:  ## Run GCC static analyzer
	$(ANALYZER_CC) $(CPPFLAGS) $(CFLAGS) -fanalyzer -fsyntax-only $(SRCS)

.PHONY: clean
clean:  ## Clean object files
	@rm -rf $(R_OBJ_DIR) $(D_OBJ_DIR) $(B_OBJ_DIR) $(BD_OBJ_DIR)

.PHONY: fclean
fclean:  ## Clean object, bin
	@$(MAKE) clean
	@rm -f $(NAME) $(NAME_D) $(NAME_B) $(NAME_B_D)

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
$(NAME): $(R_OBJECTS)
	$(CC) $(R_OBJECTS) $(R_LDFLAGS) -s -o $@
	@echo "Build complete: $(NAME) (release)"

# Debug build
$(NAME_D): $(D_OBJECTS)
	$(CC) $(D_OBJECTS) $(D_LDFLAGS) -o $@
	@echo "Build complete: $(NAME_D) (debug)"

# Bonus release build
$(NAME_B): $(B_OBJECTS)
	$(CC) $(B_OBJECTS) $(R_LDFLAGS) -s -o $@
	@echo "Build complete: $(NAME_B) (bonus release)"

# Bonus debug build
$(NAME_B_D): $(BD_OBJECTS)
	$(CC) $(BD_OBJECTS) $(D_LDFLAGS) -o $@
	@echo "Build complete: $(NAME_B_D) (bonus debug)"

# Release pattern rule
$(R_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(R_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) -c $< -o $@

# Debug pattern rule
$(D_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(D_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(D_CFLAGS) $(DEPSFLAGS) -c $< -o $@

# Bonus release pattern rule
$(B_OBJ_DIR)/%.o: $(BONUS_SRC_DIR)/%.c | $(B_OBJ_DIR)
	$(CC) $(B_CPPFLAGS) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) -c $< -o $@

# Bonus debug pattern rule
$(BD_OBJ_DIR)/%.o: $(BONUS_SRC_DIR)/%.c | $(BD_OBJ_DIR)
	$(CC) $(B_CPPFLAGS) $(CFLAGS) $(D_CFLAGS) $(DEPSFLAGS) -c $< -o $@

$(R_OBJ_DIR):
	@mkdir -p $@

$(D_OBJ_DIR):
	@mkdir -p $@

$(B_OBJ_DIR):
	@mkdir -p $@

$(BD_OBJ_DIR):
	@mkdir -p $@

-include $(R_DEPS) $(D_DEPS) $(B_DEPS) $(BD_DEPS)

# segfault: make TERM_SIZE=1231231231321
