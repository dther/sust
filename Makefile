#CCFLAGS=-Wall -Wextra -pedantic -std=gnu89
CCFLAGS=-Wall -Wextra -pedantic -std=gnu11 -g

all: sust

sust: sust.c config.h
	gcc $(CCFLAGS) $< -o $@

config.h: config.def.h
	cp $< $@
