#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <cstring>
#include <string>

#define PIPE "/tmp/pipe"

void readPipe(void){

	FILE *fp;

	int max_buf = 1000; 
	char buf[1000];

	while(1){
		int fd = open(PIPE, O_RDONLY);
		if (fd<0){
			printf("PIPE DOES NOT EXIST!!!");
			break;
		}

		ssize_t size = read(fd, buf, max_buf);
		buf[size] = '\0';
      if(strcmp(buf, "27\0") == 0){
            printf("breaking");
            break;
      }
      //print read string and close
      printf("PIPE STUFF:   %s", buf); 
      close(fd); 
  
	}

}

int main(){
    readPipe();
    return 0;
}

