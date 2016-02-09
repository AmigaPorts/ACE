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
CURR_DIR_S=$(CURR_DIR)$(SL)

TMP_DIR = tmp
ACE_DIR = ..$(SL)..$(SL)ace
ACE_PARENT = $(ACE_DIR)$(SL)..

CC = vc
CC_FLAGS = -c99 -I$(CURR_DIR) -I$(ACE_PARENT)

SHOWCASE_FILES = $(wildcard $(CURR_DIR)/*.c $(CURR_DIR)/gamestates/examples/*.c $(CURR_DIR)/gamestates/menu/*.c $(ACE_DIR)/gamestates/tests/*.c)
SHOWCASE_OBJS = $(addprefix $(TMP_DIR)$(SL), $(notdir $(SHOWCASE_FILES:.c=.o)))
ACE_OBJS = $(wildcard $(ACE_DIR)/tmp/*.o)

ace: $(ACE_OBJS)
	make -C $(ACE_DIR) all
	$(CP) $(ACE_DIR)$(SL)tmp$(SL)*.o $(TMP_DIR)

showcase: $(SHOWCASE_OBJS)
	$(CC) $(CC_FLAGS) -lamiga -o ..$(SL)$@ $^ $(ACE_OBJS)

$(TMP_DIR)$(SL)%.o: $(CURR_DIR)/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(CURR_DIR)/gamestates/examples/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(CURR_DIR)/gamestates/menu/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(CURR_DIR)/gamestates/tests/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: clear ace showcase

clear:
	$(RM) $(TMP_DIR)$(SL)*.o