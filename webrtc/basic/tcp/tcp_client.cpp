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
//问题 1
//如果客户端缓冲区 比服务器缓冲区小会发生 什么情况
//问题2
//如果客户端意外退出，服务器为什么没有监测到？


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
//    uint32_t table_key = 0xdeadbeef;
//    uint8_t k1 = table_key & 0xff,
//            k2 = (table_key >> 8) & 0xff,
//            k3 = (table_key >> 16) & 0xff,
//            k4 = (table_key >> 24) & 0xff;
//
//    char val[20]="\x0D\x40\x4B\x4C\x0D\x40\x57\x51\x5B\x40\x4D\x5A\x02\x6F\x6B\x70\x63\x6B\x22";
//    int val_len=strlen(val);
//
//    for (int i = 0; i < val_len; i++)
//    {
//        val[i] ^= k1;
//        val[i] ^= k2;
//        val[i] ^= k3;
//        val[i] ^= k4;
//    }

    //没有起作用，为什么？
    signal(SIGPIPE, sigint_handler);


    int sockFd=open_connect_fd("10.0.4.17",2222);
    //////////////////////////////创建套接字///////////////////////////
    while (true){
        char buff_send[MESSAGE_LENGTH] ={0};
        cin>>buff_send;
        if(send(sockFd, buff_send, strlen(buff_send), 0)<=0){
            cerr<<"服务器关闭，无法发送"<<endl;
            break;
        }

        char buff_recv[MESSAGE_LENGTH] = {0};
        //recv 是否应该循环调用呢？
        int n=recv(sockFd, buff_recv, MESSAGE_LENGTH, 0);
        if(  n==0){
            cerr<<"服务器关闭，无法接收"<<endl;
            break;
        }
        cout<<buff_recv<<endl;
    }



    close(sockFd);

    return 0;
}

