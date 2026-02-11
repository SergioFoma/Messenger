CXX = g++

BUILD_DIR = Build

FLAGS = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat \
-Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy \
-Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
-Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow \
-Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn \
-Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef \
-Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers \
-Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector \
-fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=30000 -Wstack-usage=8192 -pie -fPIE -Werror=vla \
-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

EXE_NAME = $(BUILD_DIR)/Server.out

SOURCES = server.c myStringFunction.c paint.c
OBJECTS = $(SOURCES:.c=.o)

OBJECTS_IN_SOURCE = $(addprefix src/, $(OBJECTS))

.PHONY: all clean

all: $(EXE_NAME)

$(EXE_NAME): makeBuild $(OBJECTS_IN_SOURCE)
	$(CXX) $(FLAGS) $(addprefix ./$(BUILD_DIR)/, $(OBJECTS_IN_SOURCE)) -o $(EXE_NAME)

$(OBJECTS_IN_SOURCE): %.o: %.c
	$(CXX) $(FLAGS) -I ./include -c ./$^ -o ./$(BUILD_DIR)/$@

makeBuild:
	mkdir -p ./$(BUILD_DIR)/src
clean:
	rm -rf ./$(BUILD_DIR)
