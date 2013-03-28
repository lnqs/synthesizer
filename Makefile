CC = clang
CFLAGS = -std=c99 -Wall -Werror -ggdb -O0 $(shell pkg-config --cflags alsa)
LDFLAGS = -lm $(shell pkg-config --libs alsa)

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = demo

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(MAKEFILE_LIST)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c %.d $(MAKEFILE_LIST)
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

clean:
	rm -rf *.o *.d $(EXECUTABLE)

-include $(SOURCES:.c=.d)
