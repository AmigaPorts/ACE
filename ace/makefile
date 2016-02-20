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

ACE_PARENT = $(ACE_DIR)$(SL)..
CC = vc
CC_FLAGS = -c99 -I$(ACE_PARENT)


TMP_DIR = tmp

ACE_FILES = $(wildcard $(ACE_DIR)$(SL)managers/*.c $(ACE_DIR)$(SL)managers/viewport/*.c $(ACE_DIR)$(SL)utils/*.c)
ACE_OBJS = $(addprefix $(TMP_DIR)$(SL), $(notdir $(ACE_FILES:.c=.o)))

hello:
	@echo.
	@echo ===============================================
	@echo ACE Full build commenced'
	@echo ===============================================
	
summary:
	@echo.
	@echo ========================
	@echo ACE Full build completed
	@echo ========================

ace: $(ACE_OBJS)
	@echo.
	@echo Copying libs...
	$(CP) libs$(SL)pario.o $(TMP_DIR)$(SL)pario.o

$(TMP_DIR)$(SL)%.o: $(ACE_DIR)$(SL)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(ACE_DIR)$(SL)managers/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(ACE_DIR)$(SL)managers/viewport/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)$(SL)%.o: $(ACE_DIR)$(SL)utils/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: hello clear ace summary

clear:
	@echo.
	@echo Removing old objs...
	$(RM) $(TMP_DIR)$(SL)*.o