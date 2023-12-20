#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include "signal.h"
using std::cin;
using std::cout;
using std::cerr;
using std::endl;


#define MESSAGE_LENGTH 512*4
void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n",sig);
    exit(0);
}




int open_connect_fd(const char* remoteIp,int remotePort){
    //////////////////////////////创建套接字///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr<<"socket error"<<endl;
        exit(1);
    }

    //////////////////////////////连接服务器///////////////////////////
    struct sockaddr_in addr;
    addr.sin_family     = AF_INET;
    addr.sin_port       = htons(remotePort);
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    unsigned int addrlen=sizeof(struct sockaddr_in);

    if(connect(sockFd, (const struct sockaddr*)&addr, addrlen) < 0){
        cerr<<"connect error"<<endl;
        exit(1);
    }
//    探测是从哪个ip发出请求的？
//    socklen_t addr_len=sizeof(addr);
//    getsockname(sockFd, (struct sockaddr *)&addr, &addr_len);
//    char *ip_str = inet_ntoa(addr.sin_addr);
//    printf("%s\n",ip_str);
    return sockFd;
}


int main(int argc, char * *argv)
{


    //没有起作用，为什么？
    signal(SIGPIPE, sigint_handler);
    FILE * fp= fopen("/Users/zhanggxk/Downloads/T-REC-H.264-201704-S!!PDF-E.pdf","rb");

    int sockFd=open_connect_fd("127.0.0.1",8765);
    //////////////////////////////创建套接字///////////////////////////
    char buff_send[MESSAGE_LENGTH] ={0};

    while (!feof(fp)){
        int n= fread(buff_send,1,MESSAGE_LENGTH,fp);

        if (n<=0){
            break;
        }
        if(send(sockFd, buff_send, n, 0)<=0){
            cerr<<"服务器关闭，无法发送"<<endl;
            break;
        }

    }
    fclose(fp);
    close(sockFd);

    return 0;
}

