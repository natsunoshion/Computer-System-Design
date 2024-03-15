NAME = nemu
INC_DIR += ./include
BUILD_DIR ?= ./build
OBJ_DIR ?= $(BUILD_DIR)/obj
BINARY ?= $(BUILD_DIR)/$(NAME)

include Makefile.git

.DEFAULT_GOAL = app

# Compilation flags
CC = gcc
LD = gcc
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -MMD -Wall -Werror -ggdb $(INCLUDES)

# Files to be compiled
SRCS = $(shell find src/ -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

# Depencies
-include $(OBJS:.o=.d)

# Some convinient rules

.PHONY: app run submit clean count
app: $(BINARY)

ARGS ?= -l $(BUILD_DIR)/nemu-log.txt

# Command to execute NEMU
NEMU_EXEC := $(BINARY) $(ARGS)

$(BINARY): $(OBJS)
	$(call git_commit, "compile")
	@echo + LD $@
	@$(LD) -O2 -o $@ $^ -lSDL2 -lreadline

run: $(BINARY)
	$(call git_commit, "run")
	$(NEMU_EXEC)

gdb: $(BINARY)
	$(call git_commit, "gdb")
	gdb -s $(BINARY) --args $(NEMU_EXEC)

clean: 
	rm -rf $(BUILD_DIR)

count:
	@git checkout pa0
	@echo "Counting lines of code in branch pa0..."
	@PA0_COUNT=$$(find . -name "*.[ch]" -not -path "./nemu/Makefile" | xargs cat | wc -l) ; \
	git checkout pa1 ; \
	echo "Counting lines of code in branch pa1..." ; \
	PA1_COUNT=$$(find . -name "*.[ch]" -not -path "./nemu/Makefile" | xargs cat | wc -l) ; \
	echo "pa0: $$PA0_COUNT lines of code" ; \
	echo "pa1: $$PA1_COUNT lines of code" ; \
	echo "Code line difference (pa1 - pa0): $$(( $$PA1_COUNT - $$PA0_COUNT ))"
