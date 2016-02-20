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
else
	# Linux/Amiga
	RM = rm
	CP = cp
	SLASH = /
	CURR_DIR = $(shell pwd)
endif
SL= $(strip $(SLASH))
SRC_DIR = $(CURR_DIR)$(SL)src

# Directories
TMP_DIR = $(SRC_DIR)$(SL)tmp
ACE_DIR = ..$(SL)ace
ACE_PARENT = $(ACE_DIR)$(SL)..

# Compiler stuff
CC = vc
CC_FLAGS = -c99 -I$(SRC_DIR) -I$(ACE_PARENT)

# File list
SHOWCASE_FILES = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/examples/*.c $(SRC_DIR)/menu/*.c $(SRC_DIR)/tests/*.c)
SHOWCASE_OBJS = $(addprefix $(TMP_DIR)$(SL), $(notdir $(SHOWCASE_FILES:.c=.o)))
ACE_OBJS = $(wildcard $(ACE_DIR)/tmp/*.o)

# 
ace: $(ACE_OBJS)
	make -C $(ACE_DIR) all
	@echo.
	@echo Copying ACE objs:
	$(CP) $(ACE_DIR)$(SL)tmp$(SL)*.o $(TMP_DIR)
	@echo.

showcase: $(SHOWCASE_OBJS)
	@echo.
	@echo Linking...
	$(CC) $(CC_FLAGS) -lamiga -o $@ $^ $(ACE_OBJS)

$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/examples/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/menu/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(SRC_DIR)/tests/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: clear ace showcase

clear:
	$(RM) $(TMP_DIR)$(SL)*.o