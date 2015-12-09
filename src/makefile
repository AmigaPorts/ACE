CC = vc

ACE_DIR =
TMP_DIR = tmp/

ACE_FILES = $(wildcard $(ACE_DIR)managers/*.c $(ACE_DIR)managers/viewport/*.c $(ACE_DIR)utils/*.c)
ACE_OBJS = $(addprefix $(TMP_DIR), $(notdir $(ACE_FILES:.c=.o)))

ace: $(ACE_OBJS)
	copy libs/pario.o $(TMP_DIR)pario.o

$(TMP_DIR)%.o: $(ACE_DIR)%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR)managers/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR)managers/viewport/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
$(TMP_DIR)%.o: $(ACE_DIR)utils/%.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
	
all: clear ace

clear:
	delete $(TMP_DIR)*.o