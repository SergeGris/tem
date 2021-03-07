
PROGNAME=emacsc

CPPFLAGS=-D_GNU_SOURCE=1
CFLAGS=-Wall -Wextra -std=gnu99 -pipe -funroll-loops -march=native

NAME=e

OBJECTS=main.o

-include $(OBJECTS:.o=.d)

all: $(PROGNAME)

OBJECTS: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -MD $< -o

$(PROGNAME): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROGNAME) $(CPPFLAGS) $(CFLAGS) -s -O2

debug:
	$(CC) $(OBJECTS) -o $(PROGNAME) $(CPPFLAGS) $(CFLAGS) -ggdb -Wpedantic -Og

install: $(PROGNAME)
	cp $< /usr/local/bin/$(NAME)
uninstall: /usr/local/bin/$(NAME)
	rm $<
