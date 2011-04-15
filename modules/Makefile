CC = gcc
CFLAGS = -Wextra -g3 -fPIC -c
SOFLAGS = -shared

FILES = rbc_splint.c rbc_spl_parse_utils.c

.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(FILES)
	$(CC) *.o $(SOFLAGS) -o libsplint.so

clean:
	rm -f *.o *~ libsplint.so

