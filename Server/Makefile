CC = gcc

BUILD_DIR = Build
COMMON_DIR = Common

LIBUV_FLAGS = $(shell pkg-config --cflags libuv)
LDLIBS = $(shell pkg-config --libs libuv)


FLAGS = -std=gnu17 -D _DEBUG -ggdb3 -O0 -Wall -Wextra \
-Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts \
-Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 \
-Wlogical-op -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow \
-Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn \
-Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
-Wstack-protector -fstack-protector -fstrict-overflow -fno-omit-frame-pointer \
-Wlarger-than=65536 -Wstack-usage=8192 -pie -fPIE -Werror=vla \
-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound

EXE_NAME = $(BUILD_DIR)/Server.out

SOURCES = server.c string_function.c chat_commands.c file_commands.c database.c
COMMON_SOURCES = server_functions.c logging.c

OBJECTS = $(SOURCES:.c=.o)
COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)

OBJECTS_IN_DIR = $(addprefix $(BUILD_DIR)/, $(OBJECTS))
COMMON_OBJECTS_IN_DIR = $(addprefix $(COMMON_DIR)/$(BUILD_DIR)/, $(COMMON_OBJECTS))

INCLUDE = -I ./include
COMMON_INCLUDE = -I Common/include

.PHONY: all clean

all: $(EXE_NAME)

$(EXE_NAME): makeBuild $(OBJECTS_IN_DIR) $(COMMON_OBJECTS_IN_DIR)
	$(CC) $(FLAGS) $(LDLIBS) $(OBJECTS_IN_DIR) $(COMMON_OBJECTS_IN_DIR) -o $(EXE_NAME)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(FLAGS) $(LIBUV_FLAGS) $(INCLUDE) $(COMMON_INCLUDE) -c $< -o $@

$(COMMON_DIR)/$(BUILD_DIR)/%.o: $(COMMON_DIR)/src/%.c
	$(CC) $(FLAGS) $(LIBUV_FLAGS) $(COMMON_INCLUDE) -c $< -o $@

makeBuild:
	mkdir -p ./$(BUILD_DIR)
	mkdir -p $(COMMON_DIR)/$(BUILD_DIR)
clean:
	rm -rf ./$(BUILD_DIR)
	rm -rf $(COMMON_DIR)/$(BUILD_DIR)
