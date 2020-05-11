#!/bin/bash

#Final Bash Script

clear
read -p "Do you want to see version 1 or 2 (enter 1 for v1 and 2 for v2)"
read version
read -p "Press Enter to start"
echo -e "Enter the height, width, and numThreads (Press enter between each entry)"
read height
read width 
read numThreads

echo "Height = $height , Width = $width and NumThreads = $numThreads"


mkfifo pipe_file

if [[ $version -eq 1 ]];then

	main=$(find ./Version_1 -type f -name "main.cpp")
	frontEnd=$(find ./Version_1 -type f -name "gl_frontEnd.cpp")
   	g++ -g3 -Wall $main $frontEnd -lm -lGL -lglut -lpthread -o test
   	./test $height $width $numThreads
     
elif [[ $version -eq 2 ]];then
	main=$(find ./Version_2 -type f -name "main.cpp")
	frontEnd=$(find ./Version_2 -type f -name "gl_frontEnd.cpp")
   	g++ -g3 -Wall $main $frontEnd -lm -lGL -lglut -lpthread -o test
   	./test $height $width $numThreads
fi

while true
do
	read input
	echo "$input" > pipe_file
	if [[ $cmd == "quit" ]]
	then
		echo "quitting program"
		break
	fi
done
	



