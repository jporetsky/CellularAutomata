#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void readPipe(void){

	char *pipe = "pipe";
	int fd;
	int max_buf = 1000; 
	char buf[1000];

	while(1){
		fd = open(pipe, O_RDONLY);
		if (fd<0){
			printf("PIPE DOES NOT EXIST!!!");
		}

		read(fd, buf, max_buf);
        if(buf = '27'){
            printf("breaking");
            break;
        }
        // Print the read string and close 
        printf(buf); 
        close(fd); 
  
	}

}

int main(){
    readPipe();
    return 0;
}

