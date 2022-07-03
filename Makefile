CCFLAGS=-Wall -Wextra -pedantic -std=gnu89
CC=gcc
#CCFLAGS=-Wall -Wextra -pedantic -std=gnu11 -g
PREFIX=/usr/local/

all: sust

sust: sust.c config.h
	$(CC) $(CCFLAGS) $< -o $@

config.h:
	cp config.def.h $@

clean:
	rm -rf sust

install: sust
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f sust $(DESTDIR)$(PREFIX)/bin/sust
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sust
