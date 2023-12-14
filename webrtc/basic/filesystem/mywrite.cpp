#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
int main(){

    int fd=open("a.txt",O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR);
    char buf[128];
    int n=0;
//    fd=1;
int c=0;
    write(fd,"a ",2);
    while (1) {
        write(fd,"a ",2);

        if (c++%10==0)
        sleep(1);

    }

    printf("\n");
    close(fd);
}