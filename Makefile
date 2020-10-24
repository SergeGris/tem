
PROGNAME=emacsc

FLAGS=-O2 -Wall -Wextra -std=c90 -ggdb

all: emacsc.c utils.c
	gcc $^ -o $(PROGNAME) $(FLAGS)

install: $(PROGNAME)
	cp $< /usr/local/bin/e
uninstall: /usr/local/bin/e
	rm $<
