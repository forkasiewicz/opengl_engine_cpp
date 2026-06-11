CXX := g++

CXXFLAGS := \
	-std=c++20 \
	-g \
	-Wall \
	-Wextra \
	-Ilib/glad/include \
	-Ilib/glm \
	-Ilib/stb

LDFLAGS := \
	-lSDL3 \
	-lGL \
	-ldl \
	-lpthread

SRC_CPP := $(wildcard src/*.cpp)
SRC_C := lib/glad/src/glad.c

OBJ_CPP := $(patsubst src/%.cpp,build/%.o,$(SRC_CPP))
OBJ_C := build/glad.o

TARGET := bin/engine

all: $(TARGET)

$(TARGET): $(OBJ_CPP) $(OBJ_C)
	mkdir -p bin
	$(CXX) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/glad.o: lib/glad/src/glad.c
	mkdir -p build
	cc -Ilib/glad/include -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf build bin

.PHONY: all run clean
