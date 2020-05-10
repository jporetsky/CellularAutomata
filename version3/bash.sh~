#!/bin/bash

# compile program
echo "compiling... " $1 $2 $3
g++ main.cpp gl_frontEnd.cpp -lm -lGL -lglut -lpthread -o cell
if [ -f cell ]
then
	echo "built cell"
fi



# Read
#pipe=/tmp/testpipe
#
#trap "rm -f $pipe" EXIT
#
#if [[ ! -p $pipe ]]; then
#    mkfifo $pipe
#fi

#while true
#do
#	if [[ "$1" ]]; then
#	    echo "$1" >$pipe
#	else
#	    echo "Hello from $$" >$pipe
#	fi
#done
#
#echo "Reader exiting"



# run program
./cell $1 $2 $3











# terminate program
#rm cell
