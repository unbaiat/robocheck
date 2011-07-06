CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wextra -g3 -fPIC -c $(CPPFLAGS)
SOFLAGS = -shared $(LDLIBS)

FILES = rbc_simian.c

.PHONY: all clean

all:	compile 
	$(CC) rbc_simian.o $(SOFLAGS) -o libsimian.so
compile:
	$(CC) $(CFLAGS) $(FILES)

clean:
	rm -f *.o *~ libsimian.so

