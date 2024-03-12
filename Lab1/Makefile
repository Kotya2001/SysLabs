GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic -I ../utils/heap_help/

all: libcoro.c solution.c mergesort.c
	gcc $(GCC_FLAGS) libcoro.c solution.c mergesort.c ../utils/heap_help/heap_help.c -o main

.PHONY: clean
clean:
	rm main
	rm out.txt