CC = gcc
CPPFLAGS = `pkg-config --cflags libxslt`
LDLIBS = `pkg-config --libs libxslt`
CFLAGS = -Wall -Wextra -g3 -c -fPIC $(CPPFLAGS)
SO_FLAGS = -shared
LIBRBC_FLAGS = -ldl $(LDLIBS)

DIR_SRC = src
XML_SRC = config

RBC_FILES = rbc_utils.c rbc_task.c librobocheck.c rbc_api.c penalty.c
RBC_FILES_PATH = $(patsubst %,$(DIR_SRC)/%,$(RBC_FILES))
RBC_OBJ_FILES = $(patsubst %.c,%.o,$(RBC_FILES))

XML_FILES = rbc_xml_parser.c rbc_config.c
XML_FILES_PATH = $(patsubst %,$(XML_SRC)/%,$(XML_FILES))
XML_OBJ_FILES = $(patsubst %.c,%.o,$(XML_FILES))

#
# Pretty print
#
V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_GEN     = $(Q:@=@echo    '     GEN      '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)

UTILS_SRC = src/utils.c
UTILS_OBJ = utils.o

.PHONY: clean all

all: utils robocheck sparse drmemory
	bash make_modules.sh build
	gcc -Wall $(CPPFLAGS) $(LDLIBS) main.c -o robocheck -lrobocheck -lutils -L.
	ln -sf $(PWD)/sparse-0.4.1/rbc_sparse_utils/black_list /tmp/black_list
	ln -sf sparse-0.4.1/rbc_sparse_utils/static_analyzer

utils: $(UTILS_OBJ)
	$(QUIET_LINK) $(CC) $(UTILS_OBJ) -shared -o libutils.so
	-rm -f $(UTILS_OBJ)

utils.o:
	$(CC) $(CFLAGS) -c -o $@ $(UTILS_SRC)

robocheck: make_obj configure penalty
	$(QUIET_LINK) $(CC) $(XML_OBJ_FILES) $(RBC_OBJ_FILES) $(LIBRBC_FLAGS) $(SO_FLAGS) -o librobocheck.so
	rm -f *.o *~

make_obj:
	$(CC) $(CFLAGS) $(XML_FILES_PATH) config/configure.c
	$(CC) $(CFLAGS) $(RBC_FILES_PATH)

configure: configure.o
	$(QUIET_LINK) $(CC) $^ $(LDLIBS) $(XML_OBJ_FILES) -o robo_config

penalty: penalty.o
	$(CC) $^ $(LDLIBS) $(SO_FLAGS) -o libpenalty.so

sparse:
	cd ./sparse-0.4.1; make; rm -f *.o

drmemory:
	cd ./drmemory-read-only; ./configure ; cd ..
	-rm -f drmemory
	ln -s ./drmemory-read-only/build/bin/drmemory.pl drmemory

clean:
	cd ./sparse-0.4.1; make clean
	-rm -f *.so *.o *~ robo_config robocheck static_analyzer /tmp/black_list
	-rm -f drmemory
	bash make_modules.sh clean
