CC = gcc

BUILD_DIR = Build
COMMON_DIR = Common

NCURSES_FLAGS = -lncurses
LIBUV_FLAGS = $(shell pkg-config --cflags libuv)
LDLIBS = $(shell pkg-config --libs libuv)

FLAGS = -std=gnu17 -D _DEBUG -ggdb3 -O0 -w
EXE_NAME = $(BUILD_DIR)/Client.out

SOURCES = client.c callbacks.c sending_files.c logger.c user_interface.c

OBJECTS = $(SOURCES:.c=.o)

OBJECTS_IN_DIR = $(addprefix $(BUILD_DIR)/, $(OBJECTS))

INCLUDE = -I ./include
COMMON_INCLUDE = -I Common/include

.PHONY: all clean

all: $(EXE_NAME)

$(EXE_NAME): makeBuild $(OBJECTS_IN_DIR)
	$(CC) $(FLAGS) $(OBJECTS_IN_DIR) -o $(EXE_NAME) $(LDLIBS) $(NCURSES_FLAGS)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(FLAGS) $(LIBUV_FLAGS) $(INCLUDE) $(COMMON_INCLUDE) -c $< -o $@

makeBuild:
	mkdir -p ./$(BUILD_DIR)
clean:
	rm -rf ./$(BUILD_DIR)
