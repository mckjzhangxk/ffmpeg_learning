#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>


#ifdef __linux__
#include <sys/epoll.h>

#elif defined(__APPLE__) && defined(__MACH__)

#define EPOLLIN
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLLIN 0x02
#define EPOLLET 11

typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event {
    uint32_t events;    /* Epoll events */
    epoll_data_t data;    /* User data variable */
};

extern int epoll_create(int __size);

extern int epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event *__event);

extern int epoll_wait(int __epfd, struct epoll_event *__events, int __maxevents, int __timeout);

#else

#endif


#define MAX_EVENTS 20
#define TIME_OUT 0
#define MESSAGE_SIZE 1024
#define PORT 9876

void set_fd_noblock(int fd) {
    int flag = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, flag | O_NONBLOCK);
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
    if (ret <= 0) {
        if (ret == 0)
            printf("客户端[%d]退出\n", accept_fd);
        return ret;
    }
    printf("receive message:%s\n", in_buf);
    send(accept_fd, (void *) in_buf, MESSAGE_SIZE, 0);
    return ret;
}


struct IO_Set {
    int epoll_fd;
    int server_fd;

    int client_num;

    struct epoll_event events[MAX_EVENTS];//当有io事件发生，epoll_wait存到到此处我们感兴趣的事件
    int timeout;
};

void init_io_set(IO_Set &ioSet, int listenFd) {
    ioSet.server_fd = listenFd;
    ioSet.client_num = 0;

    ioSet.epoll_fd = epoll_create(256);//参数不起作用

    ioSet.timeout = TIME_OUT;//ms

    struct epoll_event event;
    event.events = EPOLLIN; //表示输入
    event.data.fd = listenFd;
    //加入listenFd到 epoll监控 并设置感兴趣的事件
    if (epoll_ctl(ioSet.epoll_fd, EPOLL_CTL_ADD, listenFd, &event) < 0) {
        exit(1);
    }
}

int add_client(IO_Set &ioSet, int clientFd) {
    printf("新的客户端 %d\n", clientFd);

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;//监听输入，并且是边缘，边缘的意思是 数据读取如果没有取完，也只会触发一次
    event.data.fd = clientFd;

    if (epoll_ctl(ioSet.epoll_fd, EPOLL_CTL_ADD, clientFd, &event) < 0) {
        perror("add_client error\n");
        return -1;
    }
    ioSet.client_num++;
    return 0;

}

void remove_client(IO_Set &ioSet, int clientFd) {
    struct epoll_event event;

    //一定要先删除，再close
    if (epoll_ctl(ioSet.epoll_fd, EPOLL_CTL_DEL, clientFd, &event) < 0) {
        exit(1);
    }
    close(clientFd);
    ioSet.client_num--;
}

//没有文件描述符的限制
//工作效率不会随着文件描述符数量而改变
//epoll经过系统优化，更高效
int main() {
    IO_Set ioSet;

    //creat a tcp socket
    int socket_fd = open_listen_fd(PORT);
    init_io_set(ioSet, socket_fd);

    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(struct sockaddr_in);

    while (true) {
        //阻塞等待，直到有事件发生
        printf("客户端数 %d\n", ioSet.client_num);

        int n = epoll_wait(ioSet.epoll_fd, ioSet.events, MAX_EVENTS, 1000 * 36);

        for (int i = 0; i < n; ++i) {
            auto &e = ioSet.events[i];
            if (e.data.fd == ioSet.server_fd) {
                remote_addr_len = sizeof(struct sockaddr_in);
                int accept_fd = accept(socket_fd, (struct sockaddr *) &remote_addr, &remote_addr_len); //创建一个新连接的fd
                set_fd_noblock(accept_fd);
                if (add_client(ioSet, accept_fd) < 0) {
                    close(accept_fd);
                }

            } else if (e.events & EPOLLIN) { //客户端输入
                int result;
                do {
                    result = echo(e.data.fd);
                } while (result < 0 && errno ==EINTR );
                
                if ((result == 0) ||  (result<0&&errno!=EAGAIN) ) {
                    remove_client(ioSet, e.data.fd);
                }

            }

        }
    }

}

