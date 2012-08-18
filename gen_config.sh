#!/bin/bash

echo -e "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" > rbc_config.xml
echo -e "<appSettings>\n" >> rbc_config.xml
echo -e "  <init output=\"NULL\">\n" >> rbc_config.xml
echo -e "    <tools count=\"0\">\n" >> rbc_config.xml
echo -e "    </tools>\n" >> rbc_config.xml
echo -e "    <input/>\n" >> rbc_config.xml
echo -e "    <penalty load=\"false\" lib_path=\"libpenalty.so\"/>\n" >> rbc_config.xml
echo -e "    <err_count value=\"19\"/>" >> rbc_config.xml
echo -e "  </init>\n" >> rbc_config.xml
echo -e "  <installed_tools count=\"0\">\n" >> rbc_config.xml
echo -e "  </installed_tools\n>" >> rbc_config.xml
echo -e "  <errors>\n" >> rbc_config.xml
# Create errors
echo -e "    <err_1 id=\"1\" name=\"Memory leak\"/>" >> rbc_config.xml
echo -e "    <err_2 id=\"2\" name=\"Invalid access exception\"/>" >> rbc_config.xml
echo -e "    <err_3 id=\"3\" name=\"Uninitialized\"/>" >> rbc_config.xml
echo -e "    <err_4 id=\"4\" name=\"File descriptors error\"/>" >> rbc_config.xml
echo -e "    <err_5 id=\"5\" name=\"Data race\"/>" >> rbc_config.xml
echo -e "    <err_6 id=\"6\" name=\"Dead lock\"/>" >> rbc_config.xml
echo -e "    <err_7 id=\"7\" name=\"Unlock\"/>" >> rbc_config.xml
echo -e "    <err_8 id=\"8\" name=\"Destroy\"/>" >> rbc_config.xml
echo -e "    <err_9 id=\"9\" name=\"Condition variable\"/>" >> rbc_config.xml
echo -e "    <err_10 id=\"10\" name=\"Hold lock\"/>" >> rbc_config.xml
echo -e "    <err_11 id=\"11\" name=\"Duplicate code\"/>" >> rbc_config.xml
echo -e "    <err_12 id=\"12\" name=\"Static variable\"/>" >> rbc_config.xml
echo -e "    <err_13 id=\"13\" name=\"Signed unsigned\"/>" >> rbc_config.xml
echo -e "    <err_14 id=\"14\" name=\"Unverified function return call\"/>" >> rbc_config.xml
echo -e "    <err_15 id=\"15\" name=\"Function line count exceeds maximum admited\"/>" >> rbc_config.xml
echo -e "    <err_16 id=\"16\" name=\"Function indentation exceeds maximum admited\"/>" >> rbc_config.xml
echo -e "    <err_17 id=\"17\" name=\"Trailing whitespace\"/>" >> rbc_config.xml
echo -e "    <err_18 id=\"18\" name=\"Mixed tabs with spaces\"/>" >> rbc_config.xml
echo -e "    <err_19 id=\"19\" name=\"Invalid free\"/>" >> rbc_config.xml
echo -e "  </errors\n>" >> rbc_config.xml
echo -e "</appSettings>" >> rbc_config.xml

# Add error details
robo_config --add-error-details 1 "New memory leak modification" 1 0.2 float
robo_config --add-error-details 2 "Invalid accesses" INF 0.3 float
robo_config --add-error-details 3 "Use of unitialized variable" INF 0.2 float
robo_config --add-error-details 4 "Not closing file descriptors at the end of the program" INF 0.1 float
robo_config --add-error-details 5 "Data races" INF 0.1 float
robo_config --add-error-details 6 "Dead lock" INF 0.1 float
robo_config --add-error-details 7 "Unlock error" INF 0.1 float
robo_config --add-error-details 8 "Destroy" INF 0.1 float
robo_config --add-error-details 9 "Missusage of condition variables" INF 0.1 float
robo_config --add-error-details 10 "Hold lock" INF 0.1 float
robo_config --add-error-details 11 "Duplicate code in sources" INF 0.2 float
robo_config --add-error-details 12 "Use of non-static variables or functions in a single module" INF 0.1 float
robo_config --add-error-details 13 "Assignment from signed to unsigned" INF 0.1 float
robo_config --add-error-details 14 "Verification of return value for black listed function call is omited" INF 0.3 float
robo_config --add-error-details 15 "Function line count overlaps the maximum admited limit" INF 0.2 float
robo_config --add-error-details 16 "Indentation level overlaps the maximum admited limit" INF 0.2 float
robo_config --add-error-details 17 "Trailing whitespace at the end of lines" INF 0.2 float
robo_config --add-error-details 18 "Mixed tabs with spaces at the begining of each line" INF 0.2 float
robo_config --add-error-details 19 "Invalid memory free" INF 0.1 float

NAME=drmemory
# Install tool
robo_config --create-tool $NAME libdrmemory.so dynamic
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register parameters used when tool is running
# Register errors
robo_config --register-error 1 $NAME
robo_config --register-error 2 $NAME
robo_config --register-error 3 $NAME
robo_config --register-error 19 $NAME

NAME=valgrind
# Install tool
robo_config --create-tool $NAME libvalgrind.so dynamic
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register parameters used when tool is running
robo_config --register-parameter --trace-children=yes $NAME
robo_config --register-parameter --track-fds=yes $NAME
# Register errors
robo_config --register-error 2 $NAME
robo_config --register-error 1 $NAME
robo_config --register-error 3 $NAME
robo_config --register-error 4 $NAME
robo_config --register-error 19 $NAME

NAME=splint
# Install tool
robo_config --create-tool $NAME libsplint.so static
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register parameters used when tool is running
robo_config --register-parameter +boundswrite $NAME
robo_config --register-parameter +boundsread $NAME
# Register errors
robo_config --register-error 12 $NAME
robo_config --register-error 1 $NAME
robo_config --register-error 2 $NAME
robo_config --register-error 3 $NAME
robo_config --register-error 13 $NAME

NAME=helgrind
# Install tool
robo_config --create-tool $NAME libhelgrind.so dynamic
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register errors
robo_config --register-error 5 $NAME
robo_config --register-error 6 $NAME
robo_config --register-error 7 $NAME
robo_config --register-error 8 $NAME
robo_config --register-error 9 $NAME
robo_config --register-error 10 $NAME

NAME=simian
# Install tool
robo_config --create-tool $NAME libsimian.so static
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register parameters used when tool is running
robo_config --register-parameter -language=C $NAME
robo_config --register-parameter -threshold=4 $NAME
# Register errors
robo_config --register-error 11 $NAME

NAME=sparse
# Install tool
robo_config --create-tool $NAME libsparse.so static
# Use tool -- used at startup
robo_config --register-tool $NAME
# Register parameters used when tool is running
robo_config --register-parameter "-c" $NAME
robo_config --register-parameter "-l 150" $NAME
robo_config --register-parameter "-i 5" $NAME
robo_config --register-parameter "-s trailing" $NAME
robo_config --register-parameter "-s mixed-begin" $NAME
# Register errors
robo_config --register-error 14 $NAME
robo_config --register-error 15 $NAME
robo_config --register-error 16 $NAME
robo_config --register-error 17 $NAME
robo_config --register-error 18 $NAME

# Input configuration

config_static_files()
{
	files=$(find -name '*.c')
	for file in $files; do
		local param=$file
		if [ ${file::2} == "./" ]; then
			param=${file:2}
		fi
		robo_config --add-static-parameter $param
	done
}

robo_config --set-penalty-info true libpenalty.so
config_static_files
robo_config --set-executable ./tema0
