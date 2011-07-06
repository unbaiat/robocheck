CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = rbc_helgrind.c

.PHONY: all clean

all:	compile 
	$(CC) rbc_helgrind.o $(SOFLAGS) -o libhelgrind.so
compile:
	$(CC) $(CFLAGS) $(FILES)

clean:
	rm -f *.o *~ libhelgrind.so

