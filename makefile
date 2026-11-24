CC = gcc
CFLAGS = -Wall -Wextra

vm: vm.c
	$(CC) $(CFLAGS) -pthread vm.c -o vm

.PHONY: clean
clean:
	rm -f vm