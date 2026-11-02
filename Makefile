CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)
BIN      := csopesy

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard src/*.h) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(BIN) csopesy-log.txt

test: $(BIN)
	bash tests/smoke.sh

.PHONY: all clean test unit

unit: build/unit_process
	./build/unit_process

build/unit_process: tests/unit_process.cpp src/process.cpp src/instruction.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $^
