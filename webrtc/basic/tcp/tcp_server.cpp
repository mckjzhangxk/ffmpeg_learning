#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define MESSAGE_SIZE 1024

//bool quit= false;
void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n",sig);
    int pid;
    while(wait(&pid)>0){
        printf("回收子进程 %d\n",pid);
    }



}
//fork方式带来的问题

//1)资源可能会被长期占用
//2)分配子进程开销大
int main(){
    signal(SIGCHLD, sigint_handler);
    int ret = -1;


    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd == -1 ){
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？
    int on = 1;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if ( ret == -1 ){
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
    ret = bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
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

    //////////////////////////////接受连接///////////////////////////
    while (true){
        unsigned int addr_len = sizeof( struct sockaddr_in );

        //accept an new connection, block......
        struct sockaddr_in  remote_addr;

        int accept_fd = accept( socket_fd, (struct sockaddr *)&remote_addr, &addr_len );
        if (accept_fd<0){
            continue;
        }

        //创建子进程
        int pid = fork();

        if( pid==0 ){//子进程
            close(socket_fd);//减少引用计数
            char in_buf[MESSAGE_SIZE] = {0,};

            while (true){
                memset(in_buf, 0, MESSAGE_SIZE);
                //一直阻塞，直到新数据或者对端关闭？
                ret = recv(accept_fd ,&in_buf, MESSAGE_SIZE, 0);
                if(ret == 0){
                    printf("EOF\n");
                    break;
                }

                printf( "收到消息 :%s\n", in_buf );
                send(accept_fd, (void*)in_buf, MESSAGE_SIZE, 0);

            }

            printf("客户端退出\n");
            close(accept_fd);
            return 0;
        }else{//父进程
            close(accept_fd);//减少引用计数
        }


    }


    printf("服务器关闭 \n");
    close(socket_fd);



    return 0;
}

