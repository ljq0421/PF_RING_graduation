#include<stdio.h>  
#include<stdlib.h>  
#include<fcntl.h>  
#include<sys/types.h>  
#include<limits.h>  
#include<string.h>  
  
#define BUFS PIPE_BUF  
  
void err_quit(char *msg) {  
        printf("%s\n",msg);  
        exit(-1);  
}  
  
int main(int argc, char *argv[]){  
        int fd;  
        if ((fd = open("/home/ljq/Desktop/test0510/fifo",O_WRONLY)) < 0) {  
                err_quit("open error");  
        }  
        int id;
	char filter[100];
	//char buf[100];
	//gets(buf);
	write(fd,argv[1],strlen(buf)+1);  
	close(fd);
        return 0;  
} 
