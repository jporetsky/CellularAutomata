#!/bin/bash

pipe=/tmp/pipe

if [[ ! -p $pipe ]]; then
    echo "writer not running"
    exit 1
fi


while true
do
    if read line < "$pipe"; then
        if [[ $line == '27' ]] 
	then
            break
        fi
        echo "$line"
    fi
done

echo "Reader exiting"
