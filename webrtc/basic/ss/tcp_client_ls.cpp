#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include "signal.h"








unsigned  int get_local_addr(const char* remoteIp,int remotePort){
    //////////////////////////////创建套接字///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return 0;
    }

    //////////////////////////////连接服务器///////////////////////////
    struct sockaddr_in addr;
    addr.sin_family     = AF_INET;
    addr.sin_port       = htons(remotePort);
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    unsigned int addrlen=sizeof(struct sockaddr_in);

    if(connect(sockFd, (const struct sockaddr*)&addr, addrlen) < 0){

        return 0;
    }
    unsigned int buff_recv;
    recv(sockFd, &buff_recv, 4, 0);
    close(sockFd);
    return buff_recv;
}
int main(int argc, char * *argv)
{

    int sockFd=open_connect_fd("127.0.0.1",3443);


    return 0;
}

