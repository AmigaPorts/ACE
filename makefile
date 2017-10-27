# Windows version of VBCC requires absolute path in all .h files
# e.g. timer manager has to refer to timer.h by absolute path

#Multi-platform
ifdef ComSpec
	# Windows
	RM = del
	CP = copy
	SLASH = \\
	ACE_ROOT=$(shell chdir)
else
	# Linux/Amiga-ixemul
	RM = rm
	CP = cp
	SLASH = /
	ACE_ROOT = $(shell pwd)
endif
SL= $(strip $(SLASH))

ACE_PARENT = $(ACE_ROOT)$(SL)..

ACE_SRC_DIR = $(ACE_ROOT)$(SL)src$(SL)ace
PARIO_SRC_DIR = $(ACE_ROOT)$(SL)src$(SL)pario
FIXMATH_SRC_DIR = $(ACE_ROOT)$(SL)src$(SL)fixmath

ACE_INC_DIR = $(ACE_ROOT)$(SL)include

CC = vc
CC_FLAGS = +kick13 -c99 -I$(ACE_INC_DIR) -DAMIGA

BUILD_DIR = build

ACE_FILES = $(wildcard \
	$(ACE_SRC_DIR)$(SL)managers/*.c \
	$(ACE_SRC_DIR)$(SL)managers/viewport/*.c \
	$(ACE_SRC_DIR)$(SL)utils/*.c \
)
ACE_OBJS = $(addprefix $(BUILD_DIR)$(SL), $(notdir $(ACE_FILES:.c=.o)))

PARIO_FILES = $(wildcard $(PARIO_SRC_DIR)/*.asm)
PARIO_OBJS = $(addprefix $(BUILD_DIR)$(SL), $(notdir $(PARIO_FILES:.asm=.o)))

FIXMATH_FILES = $(wildcard $(FIXMATH_SRC_DIR)/*.c)
FIXMATH_OBJS = $(addprefix $(BUILD_DIR)$(SL), $(notdir $(FIXMATH_FILES:.c=.o)))

ace: $(ACE_OBJS) $(FIXMATH_OBJS) $(PARIO_OBJS)

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

$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)managers$(SL)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)managers$(SL)viewport$(SL)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)utils$(SL)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<

$(BUILD_DIR)$(SL)%.o: $(PARIO_SRC_DIR)$(SL)%.asm
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(BUILD_DIR)$(SL)%.o: $(FIXMATH_SRC_DIR)$(SL)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: hello clear ace summary

clear:
	@echo.
	@echo Removing old objs...
	$(RM) $(BUILD_DIR)$(SL)*.o