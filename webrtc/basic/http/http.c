#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>

int open_tcp_block_fd(const char *remoteIp, int remotePort) {
    //////////////////////////////创建套接字///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    int flag = fcntl(sockFd, F_GETFL);
    fcntl(sockFd, F_SETFL, flag | O_NONBLOCK);

    //////////////////////////////连接服务器///////////////////////////
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remotePort);
    addr.sin_addr.s_addr = inet_addr(remoteIp);
    unsigned int addrlen = sizeof(struct sockaddr_in);

//    if (connect(sockFd, (const struct sockaddr *) &addr, addrlen) < 0) {
//        return -1;
//    }
    connect(sockFd, (const struct sockaddr *) &addr, addrlen);

    return sockFd;
}

#define BUFFER(p)  (buf+ strlen(buf))

void build_header(char buf[256], char *method, char *path, char *domain) {

    sprintf(BUFFER(buf), "%s %s HTTP/1.1\r\n", method, path);
//    sprintf( BUFFER(buf),"Connection: keep-alive\r\n");
    sprintf(BUFFER(buf), "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n");
    sprintf(BUFFER(buf), "Accept-Language: en-US,en;q=0.8");
    sprintf(BUFFER(buf), "Host: %s\r\n", domain);
    sprintf(BUFFER(buf),
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36\r\n");
    sprintf(BUFFER(buf), "\r\n");
}

#define MESSAGE_LENGTH 512*6
void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n",sig);
}
enum ConnectState{
    INIT,
    CONNECTING,
    RECV,
    RESTART
};
struct http_state{
    int fd;
    enum ConnectState state;
};


int main() {
    signal(SIGPIPE, sigint_handler);

//    struct http_state http={-1,INIT};

    int socks=64;
    struct http_state http_socks[socks];
    for (int i = 0; i < socks; ++i) {
        struct http_state *http = &http_socks[i];
        http->state=INIT;
    }


    char header[512] = {0};
    build_header(header, "GET", "/", "test2:3000");
    fd_set fd_set_wr,fd_set_rd;

    while (1){
        FD_ZERO(&fd_set_wr);
        FD_ZERO(&fd_set_rd);
        int maxFd=-1;
        for (int i = 0; i < socks; ++i) {
            struct http_state* http=&http_socks[i];
            if (http->state==INIT){
                http->state=CONNECTING;
                http->fd=open_tcp_block_fd("192.168.126.126", 3000);;
                FD_SET(http->fd, &fd_set_wr);

                if (http->fd>maxFd){
                    maxFd=http->fd;
                }
            } else if(http->state==CONNECTING){
                FD_SET(http->fd, &fd_set_wr);
                if (http->fd>maxFd){
                    maxFd=http->fd;
                }
            }else if(http->state==RECV){
                FD_SET(http->fd, &fd_set_rd);
                if (http->fd>maxFd){
                    maxFd=http->fd;
                }
            } else if(http->state==RESTART){
                http->state=CONNECTING;
                FD_SET(http->fd, &fd_set_wr);
                if (http->fd>maxFd){
                    maxFd=http->fd;
                }
            }
        }

        int r=select(maxFd+ 1, &fd_set_rd, &fd_set_wr, NULL, 0);
        if (r<0)
            continue;
        for (int i = 0; i < socks; ++i) {
            struct http_state* http=&http_socks[i];
            if(FD_ISSET(http->fd,&fd_set_wr)){
                if (http->state==CONNECTING){
                    int n=send(http->fd, header, strlen(header), MSG_NOSIGNAL);
                    if (n<0){
                        http->state=INIT;
                    } else{
                        http->state=RECV;
                    }
                }
            }


            if (FD_ISSET(http->fd,&fd_set_rd)){
                if (http->state==RECV){
                    char buff_recv[MESSAGE_LENGTH] = {0};
                    int n=recv(http->fd, buff_recv, MESSAGE_LENGTH, 0);
                    if(n<=0){
                        http->state=INIT;
                    } else{
                        printf("%d\n",n);
                    }

                }

            }
        }


    }



    return 0;
}