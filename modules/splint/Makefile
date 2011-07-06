CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = rbc_splint.c

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(FILES)
	$(CC) *.o $(SOFLAGS) -o libsplint.so

clean:
	rm -f *.o *~ libsplint.so

