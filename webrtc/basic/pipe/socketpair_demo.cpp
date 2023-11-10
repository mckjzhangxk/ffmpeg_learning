#include <unistd.h>
#include <sys/socket.h>
#include <string>


int main() {

    int sv[2];
    if(socketpair(AF_UNIX,SOCK_STREAM,0,sv)<0){
        return -1;
    }

    pid_t  pid=fork();

    if (pid==0){//child
        close(sv[1]);
        while (1){
            char * msg="I am Child";
            char buffer[1024]={0};

            write(sv[0],msg, strlen(msg));
            ssize_t n=read(sv[0],buffer,1024);
            if (n>0){
                printf("%s\n",buffer);
            }
            sleep(1);
        }



    } else{
        close(sv[0]);
        while (1){
            char buffer[1024]={0};
            ssize_t n=read(sv[1],buffer,1024);
            if (n>0){
                printf("%s\n",buffer);
            }

            char * msg="I am Parent";
            write(sv[1],msg, strlen(msg));
            sleep(1);
        }

    }

    return 0;
}