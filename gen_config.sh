#!/bin/bash

# Clean old errors
cp lib/rbc_err_codes.h.bk lib/rbc_err_codes.h

echo -e "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" > rbc_config.xml
echo -e "<appSettings>\n" >> rbc_config.xml
echo -e "  <init output=\"NULL\">\n" >> rbc_config.xml
echo -e "    <tools count=\"0\">\n" >> rbc_config.xml
echo -e "    </tools>\n" >> rbc_config.xml
echo -e "    <input/>\n" >> rbc_config.xml
echo -e "    <penalty load=\"false\" lib_path=\"libpenalty.so\"/>\n" >> rbc_config.xml
echo -e "    <err_count value=\"0\"/>" >> rbc_config.xml
echo -e "  </init>\n" >> rbc_config.xml
echo -e "  <installed_tools count=\"0\">\n" >> rbc_config.xml
echo -e "  </installed_tools\n>" >> rbc_config.xml
echo -e "  <errors>\n" >> rbc_config.xml
echo -e "  </errors\n>" >> rbc_config.xml
echo -e "</appSettings>" >> rbc_config.xml

# Create errors
./configure --create-error "Memory leak"
./configure --create-error "Invalid access exception"
./configure --create-error "Uninitialized"
./configure --create-error "File descriptors error"
./configure --create-error "Data race"
./configure --create-error "Dead lock"
./configure --create-error "Unlock"
./configure --create-error "Destroy"
./configure --create-error "Condition variable"
./configure --create-error "Hold lock"
./configure --create-error "Duplicate code"
./configure --create-error "Static variable"
./configure --create-error "Signed unsigned"
./configure --create-error "Unverified function return call"
./configure --create-error "Function line count exceeds maximum admited"
./configure --create-error "Function indentation exceeds maximum admited"
./configure --create-error "Trailing whitespace"
./configure --create-error "Mixed tabs with spaces"
./configure --create-error "Invalid free"

# Add error details
./configure --add-error-details 1 "New memory leak modification" 1 0.2 float
./configure --add-error-details 2 "Invalid accesses" INF 0.3 float
./configure --add-error-details 3 "Use of unitialized variable" INF 0.2 float
./configure --add-error-details 4 "Not closing file descriptors at the end of the program" INF 0.1 float
./configure --add-error-details 5 "Data races" INF 0.1 float
./configure --add-error-details 6 "Dead lock" INF 0.1 float
./configure --add-error-details 7 "Unlock error" INF 0.1 float
./configure --add-error-details 8 "Destroy" INF 0.1 float
./configure --add-error-details 9 "Missusage of condition variables" INF 0.1 float
./configure --add-error-details 10 "Hold lock" INF 0.1 float
./configure --add-error-details 11 "Duplicate code in sources" INF 0.2 float
./configure --add-error-details 12 "Use of non-static variables or functions in a single module" INF 0.1 float
./configure --add-error-details 13 "Assignment from signed to unsigned" INF 0.1 float
./configure --add-error-details 14 "Verification of return value for black listed function call is omited" INF 0.3 float
./configure --add-error-details 15 "Function line count overlaps the maximum admited limit" INF 0.2 float
./configure --add-error-details 16 "Indentation level overlaps the maximum admited limit" INF 0.2 float
./configure --add-error-details 17 "Trailing whitespace at the end of lines" INF 0.2 float
./configure --add-error-details 18 "Mixed tabs with spaces at the begining of each line" INF 0.2 float
./configure --add-error-details 19 "Invalid memory free" INF 0.1 float

NAME=valgrind
# Install tool
./configure --create-tool $NAME ./modules/valgrind/libvalgrind.so dynamic
# Use tool -- used at startup
./configure --register-tool $NAME
# Register parameters used when tool is running
./configure --register-parameter --trace-children=yes $NAME
./configure --register-parameter --track-fds=yes $NAME
# Register errors
./configure --register-error 2 $NAME
./configure --register-error 1 $NAME
./configure --register-error 3 $NAME
./configure --register-error 4 $NAME
./configure --register-error 19 $NAME

NAME=splint
# Install tool
./configure --create-tool $NAME ./modules/splint/libsplint.so static
# Use tool -- used at startup
./configure --register-tool $NAME
# Register parameters used when tool is running
./configure --register-parameter +boundswrite $NAME
./configure --register-parameter +boundsread $NAME
# Register errors
./configure --register-error 12 $NAME
./configure --register-error 1 $NAME
./configure --register-error 2 $NAME
./configure --register-error 3 $NAME
./configure --register-error 13 $NAME

NAME=helgrind
# Install tool
./configure --create-tool $NAME ./modules/helgrind/libhelgrind.so dynamic
# Use tool -- used at startup
./configure --register-tool $NAME
# Register errors
./configure --register-error 5 $NAME
./configure --register-error 6 $NAME
./configure --register-error 7 $NAME
./configure --register-error 8 $NAME
./configure --register-error 9 $NAME
./configure --register-error 10 $NAME

NAME=simian
# Install tool
./configure --create-tool $NAME ./modules/simian/libsimian.so static
# Use tool -- used at startup
./configure --register-tool $NAME
# Register parameters used when tool is running
./configure --register-parameter -language=C $NAME
./configure --register-parameter -threshold=4 $NAME
# Register errors
./configure --register-error 11 $NAME

NAME=sparse
# Install tool
./configure --create-tool $NAME ./modules/sparse/libsparse.so static
# Use tool -- used at startup
./configure --register-tool $NAME
# Register parameters used when tool is running
./configure --register-parameter "-c" $NAME
./configure --register-parameter "-l 150" $NAME
./configure --register-parameter "-i 5" $NAME
./configure --register-parameter "-s trailing" $NAME
./configure --register-parameter "-s mixed-begin" $NAME
# Register errors
./configure --register-error 14 $NAME
./configure --register-error 15 $NAME
./configure --register-error 16 $NAME
./configure --register-error 17 $NAME
./configure --register-error 18 $NAME

# Input configuration
./configure --set-executable ./tests/complex
./configure --add-static-parameter tests/complex_test.c
./configure --set-penalty-info true libpenalty.so
