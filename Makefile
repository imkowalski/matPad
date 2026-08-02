.PHONY: all build clean run

all: build
	@cmake --build build

build:
	@cmake -B build

clean:
	@rm -rf build

run: clean all 
	@./build/matlab-lite
	