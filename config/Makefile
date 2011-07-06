CPPFLAGS = `pkg-config --cflags libxslt`
CFLAGS = -Wall -g
LDLIBS = `pkg-config --libs libxslt`

.PHONY: all clean

all: configure

configure: rbc_xml_parser.o rbc_config.o configure.o

clean:
	-rm -f configure
