#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>

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
//问题 1
//如果客户端缓冲区 比服务器缓冲区小会发生 什么情况
//问题2
//如果客户端意外退出，服务器为什么没有监测到？

int main(int argc, char * *argv)
{
    //没有起作用，为什么？
    signal(SIGPIPE, sigint_handler);
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
    addr.sin_port       = htons(8888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    unsigned int addrlen=sizeof(struct sockaddr_in);

    if(connect(sockFd, (const struct sockaddr*)&addr, addrlen) < 0){
        cerr<<"connect error"<<endl;
        exit(1);
    }
    //////////////////////////////创建套接字///////////////////////////
    while (true){
        char buff_send[MESSAGE_LENGTH] ={0};
        cin>>buff_send;
        if(send(sockFd, buff_send, strlen(buff_send), 0)<=0){
            cerr<<"服务器关闭，服务发生"<<endl;
            break;
        }

        char buff_recv[MESSAGE_LENGTH] = {0};
        //recv 是否应该循环调用呢？
        int n=recv(sockFd, buff_recv, MESSAGE_LENGTH, 0);
        if(  n==0){
            cerr<<"服务器关闭，服务接收"<<endl;
            break;
        }
        cout<<buff_recv<<endl;
    }



    close(sockFd);

    return 0;
}

