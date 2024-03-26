#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define FD_SIZE 2
#define MESSAGE_SIZE 1024


void set_fd_noblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int open_listen_fd(int port) {

    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？
    int on = 1;
    int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
        exit(1);
    }

    //设置 套接字为非阻塞 NONBLOCK
    set_fd_noblock(socket_fd);

    //////////////////////////////绑定套接字///////////////////////////
    //绑定的意思我理解为 让这个文件描述socket_fd 符和操作系统的 ip,port产生关联.

    //set local address
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0
    bzero(&(local_addr.sin_zero), 8);

    //bind socket
    //sockaddr与sockaddr_in是完全等价的，sockaddr_in是针对程序员类型，sockaddr是函数使用的类型
    ret = bind(socket_fd, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }
    //////////////////////////////监听套接字///////////////////////////
    int backlog = 10; //并发缓冲区大小，可以理解为并发量
    ret = listen(socket_fd, backlog);
    if (ret == -1) {
        perror("listen error");
        exit(1);
    }
    return socket_fd;
}

//读取客户端消息，然后马上会写到客户端
int echo(int accept_fd) {
    char in_buf[MESSAGE_SIZE];
    memset(in_buf, 0, MESSAGE_SIZE);
    int ret = recv(accept_fd, &in_buf, MESSAGE_SIZE, 0);
    if (ret == 0) {
        printf("客户端[%d]退出\n", accept_fd);
        return -1;
    }
    printf("receive message:%s\n", in_buf);
    send(accept_fd, (void *) in_buf, MESSAGE_SIZE, 0);
    return 0;
}


struct IO_Set {
    fd_set fd_ready_sets; //这里是给select使用，让select去修改
    fd_set fd_sets;  //这里是新增 与删除使用的set

    int nReady;  //多少fd准备好，至少有1个字节的数据

    int max_pos;  //accept_fds中最大有效位置
    int max_fd;

    int server_fd;
    int client_fds[FD_SIZE];
    int client_num;
};

void init_io_set(IO_Set &ioSet, int listenFd) {

    FD_ZERO(&ioSet.fd_sets);
    FD_SET(listenFd, &ioSet.fd_sets);

    ioSet.server_fd = ioSet.max_fd = listenFd;
    ioSet.max_pos = -1;

    for (int i = 0; i < FD_SIZE; ++i) {
        ioSet.client_fds[i] = -1;
    }
    ioSet.client_num = 0;
}

int add_client(IO_Set &ioSet, int clientFd) {
    printf("新的客户端 %d\n", clientFd);
    ioSet.nReady--;
    for (int i = 0; i < FD_SIZE; ++i) {
        if (ioSet.client_fds[i] == -1) {
            //描述符集 更新
            ioSet.client_fds[i] = clientFd;
            if (i > ioSet.max_pos) {
                ioSet.max_pos = i;
            }
            //监听集 更新
            FD_SET(clientFd, &ioSet.fd_sets);
            if (clientFd > ioSet.max_fd) {
                ioSet.max_fd = clientFd;
            }

            ioSet.client_num++;
            return 0;
        }
    }
    printf("无法服务更多客户端,当前 %d个客户端 \n", ioSet.client_num);
    return -1;

}

//可以理解成处理请求，删除断开的客户端
void check_clients(IO_Set &ioSet) {
    //记录被移出的clientfd的最大数值 与最大位置
    int max_pos = -1;
    int max_fd = -1;

    for (int i = 0; (i <= ioSet.max_pos) && (ioSet.nReady); ++i) {
        int clientfd = ioSet.client_fds[i];

        if (clientfd != -1 && FD_ISSET(clientfd, &ioSet.fd_ready_sets)) {
            ioSet.nReady--;

            int result = echo(clientfd);
            //说明客户端关闭
            if (result < 0) {
                close(clientfd);
                //描述符集 移除
                ioSet.client_fds[i] = -1;
                max_pos = i;

                //监听集 移除
                FD_CLR(clientfd, &ioSet.fd_sets);
                if (max_fd < clientfd) {
                    max_fd = clientfd;
                }

                ioSet.client_num--;
            }
        }
    }
    //如果移除的是最后一个描述符
    if (max_pos == ioSet.max_pos) {
        ioSet.max_pos = ioSet.max_pos - 1;
    }
    //如果移除的是最大的描述符
    if (max_fd == ioSet.max_fd) {
        ioSet.max_fd = ioSet.server_fd;
        for (int i = 0; i <= ioSet.max_pos; ++i) {
            int clientfd = ioSet.client_fds[i];
            if (ioSet.max_fd < clientfd) {
                ioSet.max_fd = clientfd;
            }
        }
    }
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
int main() {
    int socket_fd = open_listen_fd(8888);


    IO_Set ioSet;
    init_io_set(ioSet, socket_fd);


    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    while (true) {

        ////////////////设置监听的IO集///////////////
        ioSet.fd_ready_sets = ioSet.fd_sets;
        printf("最大fd %d,客户端数 %d\n", ioSet.max_fd, ioSet.client_num);
        //////////////////////////////////////////////////////
        ioSet.nReady = select(ioSet.max_fd + 1, &ioSet.fd_ready_sets, NULL, NULL, 0);
        if (ioSet.nReady < 0) {
            perror("select error");
            break;
        } else if (ioSet.nReady == 0) {
            printf("select time out ......");
            continue;
        } else {
            //客户端新连接
            if (FD_ISSET(socket_fd, &ioSet.fd_ready_sets)) {
                remote_addr_len = sizeof(struct sockaddr_in);
                int accept_fd = accept(socket_fd, (struct sockaddr *) &remote_addr, &remote_addr_len); //创建一个新连接的fd
                set_fd_noblock(accept_fd);

                if (add_client(ioSet, accept_fd) < 0) {
                    close(accept_fd);
                }
            }
            //客户端输入
            check_clients(ioSet);

        }
    }

    printf("quit server...\n");
    close(socket_fd);

    return 0;
}

