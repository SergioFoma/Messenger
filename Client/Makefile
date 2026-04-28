CC = gcc

BUILD_DIR = Build
COMMON_DIR = Common

NCURSES_FLAGS = -lncurses
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

EXE_NAME = $(BUILD_DIR)/Client.out

SOURCES = client.c callbacks.c sending_files.c logger.c user_interface.c

OBJECTS = $(SOURCES:.c=.o)

OBJECTS_IN_DIR = $(addprefix $(BUILD_DIR)/, $(OBJECTS))

INCLUDE = -I ./include
COMMON_INCLUDE = -I Common/include

.PHONY: all clean

all: $(EXE_NAME)

$(EXE_NAME): makeBuild $(OBJECTS_IN_DIR)
	$(CC) $(FLAGS) $(LDLIBS) $(NCURSES_FLAGS) $(OBJECTS_IN_DIR) -o $(EXE_NAME)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(FLAGS) $(LIBUV_FLAGS) $(INCLUDE) $(COMMON_INCLUDE) -c $< -o $@

makeBuild:
	mkdir -p ./$(BUILD_DIR)
clean:
	rm -rf ./$(BUILD_DIR)
