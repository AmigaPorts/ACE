# Windows version of VBCC requires absolute path in all .h files
# e.g. timer manager has to refer to timer.h by absolute path

#Multi-platform
ifdef ComSpec
	# Windows
	RM = del
	CP = copy
	SLASH = \\
	ACE_DIR=$(shell chdir)
else
	# Linux/Amiga-ixemul
	RM = rm
	CP = cp
	SLASH = /
	ACE_DIR = $(shell pwd)
endif
SL= $(strip $(SLASH))
ACE_DIR_S=$(ACE_DIR)$(SL)

CC = vc
CC_FLAGS = -c99 -I$(ACE_DIR)


TMP_DIR = tmp$(SL)

ACE_FILES = $(wildcard $(ACE_DIR_S)managers/*.c $(ACE_DIR_S)managers/viewport/*.c $(ACE_DIR_S)utils/*.c)
ACE_OBJS = $(addprefix $(TMP_DIR), $(notdir $(ACE_FILES:.c=.o)))

ace: $(ACE_OBJS)
	$(CP) libs$(SL)pario.o $(TMP_DIR)pario.o

$(TMP_DIR)%.o: $(ACE_DIR_S)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR_S)managers/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR_S)managers/viewport/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR_S)utils/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: clear ace

clear:
	$(RM) $(TMP_DIR)*.o