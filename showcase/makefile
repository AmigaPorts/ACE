# Windows version of VBCC requires absolute path in all .h files
# e.g. timer manager has to refer to timer.h by absolute path

# TODO: refactor so 'make' without args won't recompile whole ACE

# Multi-platform
ifdef ComSpec
	# Windows
	RM = del
	CP = copy
	SLASH = \\
	CURR_DIR=$(shell chdir)
	ECHO = @echo
	NEWLINE = @echo.
else
	# Linux/Amiga
	RM = rm
	CP = cp
	SLASH = /
	CURR_DIR = $(shell pwd)
	ECHO = @echo
	NEWLINE = @echo " "
endif
SL= $(strip $(SLASH))
SRC_DIR = $(CURR_DIR)$(SL)src

# Directories
TMP_DIR = $(CURR_DIR)$(SL)build
ACE_DIR = ..

# Compiler stuff
SHOWCASE_CC ?= vc

INCLUDES = -I$(SRC_DIR) -I$(ACE_DIR)/include
ifeq ($(SHOWCASE_CC), vc)
	CC_FLAGS = +kick13 -c99 $(INCLUDES) -DAMIGA
	ACE_AS = vc
	AS_FLAGS = +kick13 -c
else ifeq ($(SHOWCASE_CC), m68k-amigaos-gcc)
	CC_FLAGS = -std=gnu11 $(INCLUDES) -DAMIGA -noixemul -Wall
	ACE_AS = vasm
	AS_FLAGS = -quiet -x -m68010 -Faout
endif

# File list
SHOWCASE_MAIN_FILES = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/examples/*.c $(SRC_DIR)/menu/*.c)
SHOWCASE_MAIN_OBJS = $(addprefix $(TMP_DIR)$(SL), $(notdir $(SHOWCASE_MAIN_FILES:.c=.o)))

SHOWCASE_TEST_FILES = $(wildcard $(SRC_DIR)/test/*.c)
SHOWCASE_TEST_OBJS = $(addprefix $(TMP_DIR)$(SL)test_, $(notdir $(SHOWCASE_TEST_FILES:.c=.o)))

SHOWCASE_FILES = $(SHOWCASE_MAIN_FILES) $(SHOWCASE_TEST_FILES)
SHOWCASE_OBJS = $(SHOWCASE_MAIN_OBJS) $(SHOWCASE_TEST_OBJS)
ACE_OBJS = $(wildcard $(ACE_DIR)/build/*.o)

#
ace: $(ACE_OBJS)
	-make -C $(ACE_DIR) all ACE_CC=$(SHOWCASE_CC)
	$(NEWLINE)
	$(ECHO) Copying ACE objs
	@$(CP) $(ACE_DIR)$(SL)build$(SL)*.o $(TMP_DIR) > NUL
	$(NEWLINE)

showcase: $(SHOWCASE_OBJS)
	$(NEWLINE)
	$(ECHO) Linking...
	@$(SHOWCASE_CC) $(CC_FLAGS) -lamiga -o $@ $^ $(ACE_OBJS)

# Main files
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/%.c
	$(ECHO) Building $<
	@$(SHOWCASE_CC) $(CC_FLAGS) -c -o $@ $<

# Menu
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/menu/%.c
	$(ECHO) Building $<
	@$(SHOWCASE_CC) $(CC_FLAGS) -c -o $@ $<

# Tests
$(TMP_DIR)$(SL)test_%.o: $(SRC_DIR)/test/%.c
	$(ECHO) Building $<
	@$(SHOWCASE_CC) $(CC_FLAGS) -c -o $@ $<

# Examples
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/examples/%.c
	$(ECHO) Building $<
	@$(SHOWCASE_CC) $(CC_FLAGS) -c -o $@ $<

all: clear ace showcase

clear:
	$(ECHO) "a" > $(TMP_DIR)$(SL)foo.o
	@$(RM) $(TMP_DIR)$(SL)*.o
