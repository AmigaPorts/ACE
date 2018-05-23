# Windows version of VBCC requires absolute path in all .h files
# e.g. timer manager has to refer to timer.h by absolute path

#Multi-platform
ifdef ComSpec
	# Windows
	RM = @del
	CP = copy
	SLASH = \\
	ACE_ROOT=$(shell chdir)
	ECHO = @echo
	NEWLINE = @echo.
else
	# Linux/Amiga-ixemul
	RM = @rm
	CP = cp
	SLASH = /
	ACE_ROOT = .
	ECHO = @echo
	NEWLINE = @echo " "
endif
SL= $(strip $(SLASH))

ACE_SRC_DIR = $(ACE_ROOT)$(SL)src$(SL)ace
FIXMATH_SRC_DIR = $(ACE_ROOT)$(SL)src$(SL)fixmath
ACE_INC_DIR = $(ACE_ROOT)$(SL)include

ACE_CC ?= vc

TARGET_DEFINES =
TARGET ?= release
ifeq ($(TARGET), debug)
	TARGET_DEFINES += -DACE_DEBUG
endif

CC_FLAGS_COMMON = -I$(ACE_INC_DIR) -DAMIGA $(TARGET_DEFINES)
ifeq ($(ACE_CC), vc)
  CC_FLAGS = +kick13 -c99 $(CC_FLAGS_COMMON)
	ACE_AS = vc
	AS_FLAGS = +kick13 -c
	OBJDUMP =
else ifeq ($(ACE_CC), m68k-amigaos-gcc)
	CC_FLAGS_NO_O = $(CC_FLAGS_COMMON) -std=gnu11 -DAMIGA \
		-noixemul -Wall -Wextra -fomit-frame-pointer
	CC_FLAGS = $(CC_FLAGS_NO_O) $(TARGET_DEFINES) -O3
	#-fverbose-asm
	ACE_AS = vasmm68k_mot
	AS_FLAGS = -quiet -x -m68010 -Faout -ID:\prg\kompilatory\bebbo\m68k-amigaos\sys-include
	OBJDUMP = m68k-amigaos-objdump -S -l -D $@ > $@.dasm
endif

BUILD_DIR = build

ACE_FILES = $(wildcard \
	$(ACE_SRC_DIR)$(SL)managers/*.c \
	$(ACE_SRC_DIR)$(SL)managers/viewport/*.c \
	$(ACE_SRC_DIR)$(SL)utils/*.c \
)
ACE_OBJS = $(addprefix $(BUILD_DIR)$(SL), $(notdir $(ACE_FILES:.c=.o)))

FIXMATH_FILES = $(wildcard $(FIXMATH_SRC_DIR)/*.c)
FIXMATH_OBJS = $(addprefix $(BUILD_DIR)$(SL), $(notdir $(FIXMATH_FILES:.c=.o)))

ace: $(ACE_OBJS) $(FIXMATH_OBJS)

hello:
	$(NEWLINE)
	$(ECHO) ===============================================
	$(ECHO) ACE Full build commenced
	$(ECHO) ===============================================

summary:
	$(NEWLINE)
	$(ECHO) ========================
	$(ECHO) ACE Full build completed
	$(ECHO) ========================

# Should something go bad on VBCC/GCC, use this to pinpoint file which doesn't
# work with optimizations.
# GCC:
#   Show Bebbo optimizations:  -fbbb=+v
#   Disable Bebbo optimizations: -fbbb=-
# $(BUILD_DIR)$(SL)blit.o: $(ACE_SRC_DIR)$(SL)managers$(SL)blit.c
# 	$(ECHO) Building $< with -O1
# 	@$(ACE_CC) $(CC_FLAGS_NO_O) -O1 -c -o $@ $<
# 	@$(OBJDUMP)
# For GCC assembly dump use:
# @$(ACE_CC) $(CC_FLAGS) -S -o $@.s $<

$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)managers$(SL)%.c
	$(ECHO) Building $<
	@$(ACE_CC) $(CC_FLAGS) -c -o $@ $<

$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)managers$(SL)viewport$(SL)%.c
	$(ECHO) Building $<
	@$(ACE_CC) $(CC_FLAGS) -c -o $@ $<

$(BUILD_DIR)$(SL)%.o: $(ACE_SRC_DIR)$(SL)utils$(SL)%.c
	$(ECHO) Building $<
	@$(ACE_CC) $(CC_FLAGS) -c -o $@ $<

$(BUILD_DIR)$(SL)%.o: $(FIXMATH_SRC_DIR)$(SL)%.c
	$(ECHO) Building $<
	@$(ACE_CC) $(CC_FLAGS) -c -o $@ $<

all: hello clear ace summary

clear:
	$(NEWLINE)
	@mkdir -p $(BUILD_DIR)
	$(ECHO) Removing old objs...
	$(ECHO) "a" > $(BUILD_DIR)$(SL)foo.o
	$(ECHO) "a" > $(BUILD_DIR)$(SL)foo.s
	$(ECHO) "a" > $(BUILD_DIR)$(SL)foo.dasm
	$(RM) $(BUILD_DIR)$(SL)*.o
	$(RM) $(BUILD_DIR)$(SL)*.s
	$(RM) $(BUILD_DIR)$(SL)*.dasm
