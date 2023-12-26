#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void set_fd_noblock(int fd) {
    int flag = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, flag | O_NONBLOCK);
}

//服务端套接字
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

//客户端套接字
int open_connect_fd(const char *remoteIp, int remotePort) {
    //////////////////////////////创建套接字///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error\n");
        return -1;
    }

    //////////////////////////////连接服务器///////////////////////////
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remotePort);
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    unsigned int addrlen = sizeof(struct sockaddr_in);

    if (connect(sockFd, (const struct sockaddr *) &addr, addrlen) < 0) {
        perror("socket error\n");
        return -1;
    }
    set_fd_noblock(sockFd);
    return sockFd;
}

bool receive_data(int recv_fd, char *in_buf, int in_buf_size, int &out_n) {
    while (true) {
        out_n = recv(recv_fd, in_buf, in_buf_size, 0);

        if (out_n > 0) {
            return true;
        } else {
            printf("EOF\n");
            return false;
        }
    }
}

bool send_data(int send_fd, char *in_buf, int in_n) {
    if (in_n==0){
        return true;
    }
    if (send(send_fd, in_buf, in_n, 0) <= 0) {
        printf("服务器关闭，无法发送\n");
        return false;
    }
    return true;
}


bool redirect_data(int recv_fd, int send_fd, char *in_buf, int in_buf_size, int &out_n) {
    if (!receive_data(recv_fd, in_buf, in_buf_size, out_n)) {
        return false;
    }
    if (send(send_fd, in_buf, out_n, 0) <= 0) {
        printf("服务器关闭，无法发送\n");
        return false;
    }
    return true;
}