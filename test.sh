#!/bin/bash
if [ $# -ne 2 ]; then
 echo Invalid number of arguments. Usage: $0 \<input file\> \<output file\>
 exit 1
fi
buffersize=$((($RANDOM % 5000) + 1))
echo Running executable with buffers of size $buffersize
printed="$(echo $buffersize | ./encrypt $1 $2)"
inputlines="$(echo "$printed" | sed -n '/^Input file contains$/,/^Output file contains$/{/^Input file contains$/d;/^Output file contains$/d;p}')"
for i in {A..Z}; do inputlinecounts=$(echo $inputlinecounts $i:$((0+$(echo "$inputlines" | sed -n "s/.*$i:\([0-9]\{1,\}\).*/\1/p" | xargs -n 1 echo +)))); done
echo Input file contains \(counted\)
echo $inputlinecounts
input=$(cat "$1" | tr a-z A-Z)
for i in {A..Z}; do inputcounts=$(echo $inputcounts $i:$(echo $input | grep -o $i | wc -l)); done
echo Input file contains \(actual\)
echo $inputcounts
outputlines="$(echo "$printed" | sed -n '/^Output file contains$/,/^Reset finished$/{/^Output file contains$/d;/^Reset finished$/d;/^End of file reached$/d;p}')"
for i in {A..Z}; do outputlinecounts=$(echo $outputlinecounts $i:$((0+$(echo "$outputlines" | sed -n "s/.*$i:\([0-9]\{1,\}\).*/\1/p" | xargs -n 1 echo +)))); done
echo Output file contains \(counted\)
echo $outputlinecounts
output=$(cat "$2")
for i in {A..Z}; do outputcounts=$(echo $outputcounts $i:$(echo $output | grep -o $i | wc -l)); done
echo Output file contains \(actual\)
echo $outputcounts