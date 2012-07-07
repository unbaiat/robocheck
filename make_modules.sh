#!/bin/bash

if [ $# -ne 1 ] ; then
	echo "Insufficient number of parameters."
	echo "Try <script> [build] or [clean]"
	exit
fi

if [ $1 == "build" ]
then
	for i in $( ls modules)
	do
		cd modules/$i;
		echo "Building: " $i
		make clean; make;
		if [ $? -ne 0 ]; then
			exit 1
		fi
		cd ../..;
	done

	exit
fi

if [ $1 == "clean" ]
then
	for i in $( ls modules)
	do
		cd modules/$i;
		echo "Cleaning: " $i
		make clean;
		cd ../..;
	done

	exit
fi

echo "Invalid argument. Try [build] or [clean]"

