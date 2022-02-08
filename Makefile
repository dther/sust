#CCFLAGS=-Wall -Wextra -pedantic -std=gnu89
CCFLAGS=-Wall -Wextra -pedantic -std=gnu11 -g
PREFIX=/usr/local/

all: sust

sust: sust.c config.h
	gcc $(CCFLAGS) $< -o $@

config.h: config.def.h
	cp $< $@

install: sust
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f sust $(DESTDIR)$(PREFIX)/bin/sust
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sust
