#!/bin/bash


################## steps ###########################################
# - bash make the main pipe									
# - launch process with new pipe attached
# - thread dedicated to listening on pipe
# - pipe thread init between gui init and thread init in init app
#
#
####################################################################

# delete pipe file on exit
trap "rm -f /tmp/pipe" EXIT
trap "rm -f cell" EXIT
trap "rm -f pipeTest" EXIT

rm -f /tmp/pipe
# make the pipe
mkfifo /tmp/pipe
PIPE=/tmp/pipe

# compile program
g++ main.cpp gl_frontEnd.cpp -lm -lGL -lglut -lpthread -o cell
if [ -f cell ]
then	
	echo "built cell"
	./cell $1 $2 $3
fi

## for testing
# g++ read_test.cpp -o pipeTest
# if [ -f pipeTest ]
# then	
# 	echo "built pipeTest"
# 	./pipeTest
# fi


while true
do
	# write to pipe`
	read cmd
	echo "$cmd" > "$PIPE"
	if [[ $cmd == 'end' ]]
        then
    	    break
        fi
done
