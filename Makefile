CC = gcc
COMPILE_FLAGS = -Wall -g -Wextra
LIB_FLAGS = -fPIC
SO_FLAGS = -shared
LIBRBC_FLAGS = -ldl

#** here compiled any time
all: rbc_dependency
	$(CC) librobocheck.o -o librobocheck.so $(LIBRBC_FLAGS) -lpenalty -L. -shared

rbc_dependency: libpenalty librobocheck.c
	$(CC) $(COMPILE_FLAGS) -c librobocheck.c

#** user compiles any time he wants to add new penlty functions
#   without compiling the entire application	
libpenalty: rbc_penalty.c
	$(CC) $(COMPILE_FLAGS) $(LIB_FLAGS) -c $^
	$(CC) $(SO_FLAGS) rbc_penalty.o -o libpenalty.so

#** this is module specific independent from the rest of the application
module: dependency
	$(CC) $(SO_FLAGS) rbc_spl_parse_utils.o rbc_splint.o -o libsplint.so
	
dependency: rbc_spl_parse_utils.c rbc_splint.c
	$(CC) $(COMPILE_FLAGS) $(LIB_FLAGS) -c $^

#** clean-up
.PHONY: clean

clean:
	rm -f *.so *.o *~
