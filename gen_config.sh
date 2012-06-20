#!/bin/bash

echo -e "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" > rbc_config.xml
echo -e "<appSettings>\n" >> rbc_config.xml
echo -e "  <init output=\"NULL\">\n" >> rbc_config.xml
echo -e "    <tools count=\"0\">\n" >> rbc_config.xml
echo -e "    </tools>\n" >> rbc_config.xml
echo -e "  </init>\n" >> rbc_config.xml
echo -e "  <installed_tools count=\"0\">\n" >> rbc_config.xml
echo -e "  </installed_tools\n>" >> rbc_config.xml
echo -e "  <errors>\n" >> rbc_config.xml
echo -e "  </errors\n>" >> rbc_config.xml
echo -e "</appSettings>" >> rbc_config.xml

# Install valgrind tool
./configure --create-tool valgrind ./modules/valgrind/libvalgrind.so dynamic
# Use valgrind tool -- used at startup
./configure --register-tool valgrind
