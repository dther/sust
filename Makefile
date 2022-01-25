#CCFLAGS=-Wall -Wextra -pedantic -std=gnu89
CCFLAGS=-Wall -Wextra -pedantic -std=gnu11

all: sust

sust: sust.c
	gcc $(CCFLAGS) $< -o $@
