all: msglite/msglite.cpp test/test.cpp
	@mkdir -p output/
	gcc -std=c++11 -g -Wall -Wextra -Wpedantic -I./msglite $^ -o output/test
	./output/test

big-endian: msglite/msglite.cpp test/test.cpp
	@mkdir -p output/
	mips-linux-gnu-gcc -EB -static -std=c++11 -g -Wall -Wextra -Wpedantic -I./msglite $^ -o output/mips-test
	qemu-mips ./output/mips-test

format:
	@clang-format -i msglite/msglite.h msglite/msglite.cpp test/test.cpp

clean:
	@rm -rf output/
