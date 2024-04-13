all: msglite/msglite.cpp test/test.cpp
	@mkdir -p output/
	gcc -std=c++11 -g -Wall -Wextra -I./msglite $^ -o output/test
	./output/test

format:
	@clang-format -i msglite/msglite.h msglite/msglite.cpp test/test.cpp

clean:
	@rm -rf output/
