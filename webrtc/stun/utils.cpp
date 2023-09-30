#include "utils.h"
int createUDPSocket(const char * ip,int port){
    int ret;
    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？ WAIT_TIME是socket主动关闭后必须等待的时间，之后才可以与端口解开绑定
    int on = 1;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
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
    local_addr.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;//0.0.0.0
    bzero(&(local_addr.sin_zero), 8);

    //bind socket
    //sockaddr与sockaddr_in是完全等价的，sockaddr_in是针对程序员类型，sockaddr是函数使用的类型
    ret = bind(socket_fd, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }
    return socket_fd;
}

void sendMsgTo(int socketFd, struct sockaddr_in &remote_addr,std::string&msg){
    sendto(socketFd, msg.c_str(), msg.length(), 0,(struct sockaddr*)&remote_addr,sizeof(remote_addr));

}
void sendMsgTo(int socketFd,const char * ip,int port,std::string&msg){
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(ip);
    remote_addr.sin_port = htons(port);
    bzero(&(remote_addr.sin_zero), 8);

    sendMsgTo(socketFd, remote_addr,msg);
}
std::string recvMsg(int sockFd){
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len;

    char buff_recv[1024] = {0};
    int n=recvfrom(sockFd, buff_recv, 1024, 0,(struct sockaddr*)&remote_addr,&remote_addr_len);
    if(  n==0){
        exit(1);
    }

    std::string result="来自 ";
    result+=inet_ntoa(remote_addr.sin_addr);
    result+=":"+std::to_string(htons(remote_addr.sin_port));
    result+=" 的消息：";
    result+=buff_recv;

    return result;
}
