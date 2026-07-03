NAME     := ft_ls

CC       := gcc
CPPFLAGS := -DNDEBUG -D_FORTIFY_SOURCE=3 -I include -D_DEFAULT_SOURCE
CFLAGS   := -std=c11 -O3 -march=native                                         \
			-fomit-frame-pointer -fPIE -fstack-clash-protection                \
			-fstack-protector-strong                                           \
			-Wall -Wextra -Werror -Wshadow -Wpedantic                          \
			-Wconversion -Wsign-conversion                                     \
			-Wformat=2 -Wformat-security                                       \
			-Wnull-dereference -Wcast-align -Wswitch-enum -Wundef              \
			-Wstrict-prototypes -Wmissing-prototypes                           \
			-Wredundant-decls -Wwrite-strings                                  \
			-Wimplicit-fallthrough -Wlogical-op                                \
			-Wcast-qual -Wduplicated-cond -Wduplicated-branches                \
			-Wvla -Walloca -Wold-style-definition                              \
			-Wbad-function-cast -Wmissing-declarations -Wstrict-overflow=5     \
		    -Wdate-time -Walloc-zero                                           \
            -Wframe-larger-than=4096

DEPFLAGS := -MMD -MP
LDFLAGS  := -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -s

SRCDIR   := src
SRCFILES := ft_arena.c ft_array.c ft_file_info.c ft_parse.c ft_printer.c       \
            ft_printer_helper.c ft_printer_list.c ft_shell_escape.c            \
			ft_sort.c ft_str.c ft_str_arena.c ft_symlink.c ft_utils.c          \
			ft_walk.c ft_walk_entry.c main.c

SRCS     := $(addprefix $(SRCDIR)/, $(SRCFILES))
OBJDIR   := obj
OBJS     := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS     := $(OBJS:.o=.d)

BLUE     := \033[36m
NC       := \033[0m

.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: clean
clean:  ## Clean object files
	@$(RM) -r $(OBJDIR)

.PHONY: fclean
fclean:  ## Clean object, bin
	@$(MAKE) clean
	@$(RM) $(NAME)

.PHONY: re
re:  ## Clean all and recompile
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: fmt
fmt:  ## Format code via clang-format
	@printf 'Format code\n'
	@find src include -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

.PHONY: help
help:  ## Get help
	@printf 'Usage: make ${BLUE}<target>${NC}\n'
	@printf 'Available targets:\n'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  ${BLUE}%-15s${NC} %s\n", $$1, $$2}' Makefile

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	@printf 'Build complete: %s (release)\n' '$(NAME)'

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

-include $(DEPS)
