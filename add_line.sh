#!/bin/bash

if [ $# -ne 4 ] ; then
	echo "Insufficient number of parameters."
	echo "Try <script> [max_enum] [inserted_enum] [comment_for_enum]  [file]"
	exit
fi

max_enum=$1
enum=$2
comment=$3
file=$4

echo $comment

line_number=$(grep -n $max_enum $file | cut -f1 -d:)

sed $line_number'i \\t'$enum',\t\t// '"$comment"'\n' $file > tmp

rm -f $file
mv tmp $file

# recompile the core
echo "Rebuilding core application..."
make robocheck &> /dev/null
echo "Done."

