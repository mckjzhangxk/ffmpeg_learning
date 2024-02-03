#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#ifdef __linux__
#include <wait.h>
#endif
#define MESSAGE_SIZE 1024

//bool quit= false;
void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n",sig);
    int pid,status;
    while( (pid=wait(&status))>0){
        printf("回收子进程 %d\n",pid);
    }
}

int open_listen_fd(){
    
    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if ( socket_fd == -1 ){
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？
    int on = 1;
    int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if ( ret == -1 ){
        perror("setsockopt error");
        exit(1);
    }
//    on=0;
//    ret=setsockopt(socket_fd, 41, 27, &on, sizeof(on));
//    if (ret == -1) {
//        perror("setsockopt ipv6 error");
//        exit(1);
//    }
    //////////////////////////////绑定套接字///////////////////////////
    //绑定的意思我理解为 让这个文件描述socket_fd 符和操作系统的 ip,port产生关联.

    //set local address
    struct sockaddr_in6 local_addr;
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(8003);
    local_addr.sin6_addr = in6addr_any;//0.0.0.0
//    bzero(&(local_addr.sin_zero), 8);

    //bind socket
    //sockaddr与sockaddr_in是完全等价的，sockaddr_in是针对程序员类型，sockaddr是函数使用的类型
    ret = bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(ret == -1 ) {
        perror("bind error");
        exit(1);
    }
    //////////////////////////////监听套接字///////////////////////////
    int backlog = 10; //并发缓冲区大小，可以理解为并发量
    ret = listen(socket_fd, backlog);
    if ( ret == -1 ){
        perror("listen error");
        exit(1);
    }
    return socket_fd;
}

void echo(int accept_fd){
    char in_buf[MESSAGE_SIZE] = {0,};

    while (1){
        memset(in_buf, 0, MESSAGE_SIZE);
        //一直阻塞，直到新数据或者对端关闭？
        int n = recv(accept_fd ,&in_buf, MESSAGE_SIZE, 0);
        if(n == 0){
            printf("EOF\n");
            break;
        }

        printf( "收到消息 :%s\n", in_buf );
        //send(accept_fd, (void*)in_buf, MESSAGE_SIZE, 0);

    }

    printf("客户端退出\n");
    close(accept_fd);
}
//fork方式带来的问题

//1)资源可能会被长期占用
//2)分配子进程开销大
int main(){
    signal(SIGCHLD, sigint_handler);

    int socket_fd=open_listen_fd();

    //////////////////////////////接受连接///////////////////////////
    while (1){
        socklen_t addr_len = sizeof( struct sockaddr_in );

        //accept an new connection, block......
        struct sockaddr_in  remote_addr;

        int accept_fd = accept( socket_fd, (struct sockaddr *)&remote_addr, &addr_len );
        if (accept_fd<0){
            continue;
        }

        echo(accept_fd);


    }

    printf("服务器关闭 \n");
    close(socket_fd);

}

