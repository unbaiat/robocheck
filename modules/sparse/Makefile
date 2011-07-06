CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = libsparse.c

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(FILES)
	$(CC) *.o $(SOFLAGS) -o libsparse.so
	rm -f *.o

clean:
	rm -f *.o *~ libsparse.so

