#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8888
#define FD_SIZE 1024
#define MESSAGE_SIZE 1024


void set_fd_noblock(int fd){
    int flag=fcntl(fd,F_GETFD);
    fcntl(fd,F_SETFD,flag|O_NONBLOCK);
}
//使用select API,以异步IO多路复用的方式 实现的服务器。
//异步IO是建立在【事件触发机制】 对IO进行处理。

//当有新的event到来时，程序需要做如下处理
//  1)遍历所有文件描述符，找出变化的描述符。
//  2)对监听socket和accept fd区别处理。
//  3)socket以非阻塞方式工作,recv,send,accept


//为什么套接字需要非阻塞方式？
//答：因为异步IO程序本质是单进程的，select发现event，处理对应的网络操作如果阻塞了，就没法再次
//回到select，自然没法监听其他的event,没法达到异步的效果！！。

//重要的API
//FD_ZERO,FD_SET,FD_ISSET
//fcntl -用于设置socket为非阻塞
//select
int main(){
    //套接字的创建与地址重用
    int flags = 1; //open REUSEADDR option
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));


    //设置 套接字为非阻塞 NONBLOCK
    set_fd_noblock(socket_fd);


    //绑定与监听，监听是tcp采用，用于维护与客户端connect??
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(local_addr.sin_zero), 8);

    //bind socket
    ret = bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    int backlog = 10;
    ret = listen(socket_fd, backlog);
    if (ret<0){
        printf("监听失败");
        return -1;
    }

    int accept_fds[FD_SIZE] = {-1, };
    for (int i = 0; i < FD_SIZE; ++i) {
        accept_fds[i]=-1;
    }

    fd_set fd_sets;
    while (true) {

        ////////////////重新设置要复用的文件描述符///////////////
        FD_ZERO(&fd_sets); //清空sets
        FD_SET(socket_fd, &fd_sets); //将socket_fd 添加到sets
        int max_fd = socket_fd;//每次都重新设置 max_fd

        int client_num=0;
        for(int k=0; k < FD_SIZE; k++){
            if (accept_fds[k]==-1)
                continue;
            if(accept_fds[k] > max_fd){
                    max_fd = accept_fds[k];
            }
            //继续向sets添加fd
            FD_SET(accept_fds[k],&fd_sets);
            client_num++;
        }
        //////////////////////////////////////////////////////

        printf("最大fd %d,客户端数 %d\n",max_fd,client_num);
        //遍历所有的fd
        int events = select( max_fd + 1, &fd_sets, NULL, NULL, NULL );
        if(events < 0) {
            perror("select");
            break;
        }else if(events == 0){
            printf("select time out ......");
            continue;
        }else if( events ){
            printf("events:%d\n", events);

            // 如果是监听的socket事件,表示 来的是新连接
            if( FD_ISSET(socket_fd, &fd_sets)){
                unsigned int addr_len = sizeof( struct sockaddr_in );
                struct sockaddr_in  remote_addr;
                int accept_fd = accept(socket_fd, (struct sockaddr *)&remote_addr, &addr_len); //创建一个新连接的fd
                if (accept_fd<0){
                    continue;
                }
                printf("新的客户端 %d\n",accept_fd);

                ////////////超找出空槽来接收///////////////////////
                int curpos = -1;
                for( int a; a < FD_SIZE; a++){
                    if(accept_fds[a] == -1){
                        curpos = a;
                        break;
                    }
                }
                if(curpos==-1){
                    printf("无法处理更多连接 !\n");
                    close(accept_fd);
                    continue;
                }
                ///////////////////////////////////

                set_fd_noblock(accept_fd);
                accept_fds[curpos] = accept_fd;
            }

            for(int j=0; j < FD_SIZE; j++ ){
                if( (accept_fds[j] != -1) && FD_ISSET(accept_fds[j], &fd_sets)){ //客户端有新的事件
                    printf("来自 [%d]的新事件，accept_fd: %d\n",j, accept_fds[j]);
                    char in_buf[MESSAGE_SIZE];
                    memset(in_buf, 0, MESSAGE_SIZE);
                    int ret = recv(accept_fds[j], &in_buf, MESSAGE_SIZE, 0);
                    if(ret == 0){
                        close(accept_fds[j]);
                        accept_fds[j] = -1;
                        printf("客户端[%d]退出\n",j);
                        continue;
                    }
                    printf( "receive message:%s\n", in_buf );
                    send(accept_fds[j], (void*)in_buf, MESSAGE_SIZE, 0);
                }
            }
        }
    }

    printf("quit server...\n");
    close(socket_fd);

    return 0;
}

