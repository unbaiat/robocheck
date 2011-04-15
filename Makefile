CC = gcc
CFLAGS = -Wall -Wextra -g -c -fPIC
SO_FLAGS = -shared
LIBRBC_FLAGS = -ldl

DIR_SRC = src

RBC_FILES = rbc_utils.c rbc_task.c librobocheck.c
RBC_FILES_PATH = $(patsubst %,$(DIR_SRC)/%,$(RBC_FILES))
RBC_OBJ_FILES = $(patsubst %.c,%.o,$(RBC_FILES))


all: make_obj
	$(CC) $(RBC_OBJ_FILES) $(LIBRBC_FLAGS) $(SO_FLAGS) -o librobocheck.so
	rm -f *.o *~
make_obj:
	$(CC) $(CFLAGS) $(RBC_FILES_PATH)

test:
	gcc -Wall main.c -o main -lrobocheck -L.


.PHONY: clean

clean:
	rm -f *.so *.o *~ main
