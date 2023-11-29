#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#define MESSAGE_SIZE 1024


//udp服务器
//与tcp最大的不同就是不用建立连接，不用监听端口
//客户端随时可以发送数据包给服务器，不论是否开启服务器

//nc -u 127.0.0.1 8888
int main() {

    int ret = -1;

    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？
    int on = 1;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
    }

    //////////////////////////////绑定套接字///////////////////////////
    //绑定的意思我理解为 让这个文件描述socket_fd 符和操作系统的 ip,port产生关联.

    //set local address
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(8888);
    local_addr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0
    bzero(&(local_addr.sin_zero), 8);

    //bind socket
    //sockaddr与sockaddr_in是完全等价的，sockaddr_in是针对程序员类型，sockaddr是函数使用的类型
    ret = bind(socket_fd, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }


    //////////////////////////////接受数据///////////////////////////
    while (true) {
        unsigned int addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in remote_addr;


        char in_buf[MESSAGE_SIZE] = {0,};


        memset(in_buf, 0, MESSAGE_SIZE);
        //一直阻塞，直到新数据或者对端关闭？

        int n=recvfrom(socket_fd, (void *) in_buf, MESSAGE_SIZE, 0, (struct sockaddr *) &remote_addr, &addr_len);
        if (n<0){
            break;
        }
        char ip_buffer[128];
        inet_ntop(AF_INET,&remote_addr.sin_addr,ip_buffer,128);
        printf("收到消息[%s:%d] 长度 %d :%s\n", ip_buffer, ntohs(remote_addr.sin_port),n,in_buf);


        sendto(socket_fd,in_buf,MESSAGE_SIZE,0, (struct sockaddr *) &remote_addr,sizeof(remote_addr));

    }


    printf("服务器关闭 \n");
    close(socket_fd);


    return 0;
}

