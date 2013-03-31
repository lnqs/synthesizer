CC = clang
CFLAGS = -std=c99 -Wall -Werror -ggdb -O0 $(shell pkg-config --cflags alsa)
LDFLAGS = -lm $(shell pkg-config --libs alsa)
STRIP = sstrip
COMPRESS = xz
EXTRACT_SCRIPT = \#!/bin/sh\ntail -c+59 "$$0"|unxz>d;chmod +x d;./d;rm d;exit

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = demo

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(EXECUTABLE).stripped
	echo '$(EXTRACT_SCRIPT)' > $@
	$(COMPRESS) -kec $< >> $@
	chmod +x $@

$(EXECUTABLE).stripped: $(EXECUTABLE).debug
	cp -f $< $@
	$(STRIP) $@

$(EXECUTABLE).debug: $(OBJECTS) $(MAKEFILE_LIST)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c %.d $(MAKEFILE_LIST)
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

clean:
	rm -rf *.o *.d $(EXECUTABLE) $(EXECUTABLE).debug $(EXECUTABLE).stripped

-include $(SOURCES:.c=.d)
