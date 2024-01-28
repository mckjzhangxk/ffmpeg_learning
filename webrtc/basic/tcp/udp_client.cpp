#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#define MESSAGE_LENGTH 512*4


//问题 1
//为什么我设置 非阻塞 没有作用？

int main(int argc, char * *argv)
{

    //////////////////////////////创建套接字///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        cerr<<"socket error"<<endl;
        exit(1);
    }
    int flag=fcntl(sockFd,F_GETFD);
    fcntl(sockFd,F_SETFD,flag|O_NONBLOCK);


    //////////////////////////////连接服务器///////////////////////////
    struct sockaddr_in addr;
    addr.sin_family     = AF_INET;
    addr.sin_port       = htons(8888);
    addr.sin_addr.s_addr = inet_addr("80.240.28.29");

    //////////////////////////////创建套接字///////////////////////////
    while (true){
        char buff_send[MESSAGE_LENGTH] ={0};
        cin>>buff_send;
        int n=sendto(sockFd, buff_send, strlen(buff_send), 0,(struct sockaddr*)&addr,sizeof(addr));
        printf("发送的数据，大小 %d\n",n);

        char buff_recv[MESSAGE_LENGTH] = {0};

        unsigned int addr_len=sizeof(addr);
        n=recvfrom(sockFd, buff_recv, MESSAGE_LENGTH, 0,(struct sockaddr*)&addr,&addr_len);
        if(  n==0){
            cerr<<"服务器关闭，服务接收"<<endl;
            break;
        }
        cout<<buff_recv<<endl;
    }



    close(sockFd);

    return 0;
}

