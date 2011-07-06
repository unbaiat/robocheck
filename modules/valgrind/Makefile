CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = rbc_valgrind.c

.PHONY: all clean

all:	compile 
	$(CC) rbc_valgrind.o $(SOFLAGS) -o libvalgrind.so
compile:
	$(CC) $(CFLAGS) $(FILES)

clean:
	rm -f *.o *~ libvalgrind.so

