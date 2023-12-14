#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
int main(){

    int fd=open("a.txt",O_RDONLY);
    char buf[16];
    int n=0;


    while ((n=read(fd,buf,128))>=0){
//        printf("%d\n",n);
        if (n==0){
//            sleep(1);
            continue;
        }

        for (int i = 0; i < n; ++i) {
            write(1,buf+i,1);
            write(1," ",1);
        }

    }
    printf("\n");
    close(fd);
}