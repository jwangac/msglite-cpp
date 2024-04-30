INCS := msglite/msglite.h
SRCS := msglite/msglite.cpp test/test.cpp

all: $(INCS) $(SRCS)
	@mkdir -p output/
	@gcc -std=c++11 -g -Wall -Wextra -Wpedantic -DMSGLITE_BOUND_CHECKING -I./msglite $(SRCS) -o output/test
	@./output/test

big-endian: $(INCS) $(SRCS)
	@mkdir -p output/
	@mips-linux-gnu-gcc -EB -static -std=c++11 -g -Wall -Wextra -Wpedantic -DMSGLITE_BOUND_CHECKING -I./msglite $(SRCS) -o output/mips-test
	@qemu-mips ./output/mips-test

format:
	@clang-format -i $(INCS) $(SRCS)

clean:
	@rm -rf output/
