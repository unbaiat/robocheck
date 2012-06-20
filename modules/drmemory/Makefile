CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = rbc_drmemory.c

.PHONY: all clean

all:	compile
	$(CC) rbc_drmemory.o $(SOFLAGS) -o libdrmemory.so
compile:
	$(CC) $(CFLAGS) $(FILES)

clean:
	rm -f *.o *~ libdrmemory.so

