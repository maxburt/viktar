CC = gcc
DEBUG = -g
DEFINES =
CFLAGS = $(DEBUG) -I/u/rchaney/Classes/cs333/Labs/Lab2 -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
	-Wmissing-declarations -Wold-style-definition \
	-Wmissing-prototypes -Wdeclaration-after-statement \
	-Wno-return-local-addr -Wunsafe-loop-optimizations \
	-Wuninitialized -Werror $(DEFINES)
PROG = viktar
INCLUDES = /u/rchaney/Classes/cs333/Labs/Lab2/viktar.h

all: $(PROG)

$(PROG): $(PROG).o
	$(CC) $(CFLAGS) -o $@ $^

$(PROG).o: $(PROG).c $(INCLUDES)
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROG) *.o *~ \#*

tar:
	tar cvfa Lab2_${LOGNAME}.tar.gz *.[ch] [mM]akefile

