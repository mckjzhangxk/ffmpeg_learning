#include "utils.h"
#include <fcntl.h>
#define MESSAGE_SIZE 1024
#define FD_SIZE 2
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
struct IO_Set {
    fd_set fd_ready_sets; //这里是给select使用，让select去修改
    fd_set fd_sets;  //这里是新增 与删除使用的set

    int max_fd;
    int client_fds[FD_SIZE];


};

void init_io_set(IO_Set &ioSet) {

    FD_ZERO(&ioSet.fd_sets);

    ioSet.max_fd = -1;

    for (int i = 0; i < FD_SIZE; ++i) {
        ioSet.client_fds[i] = -1;
    }
}

int add_socket(IO_Set &ioSet, int clientFd) {
    printf("新的客户端 %d\n", clientFd);

    for (int i = 0; i < FD_SIZE; ++i) {
        if (ioSet.client_fds[i] == -1) {
            //描述符集 更新
            ioSet.client_fds[i] = clientFd;

            //监听集 更新
            FD_SET(clientFd, &ioSet.fd_sets);
            if (clientFd > ioSet.max_fd) {
                ioSet.max_fd = clientFd;
            }
            return 0;
        }
    }
    return -1;

}


void echo(int socket_fd){
    unsigned int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in remote_addr;


    char in_buf[MESSAGE_SIZE] = {0,};
    int n=recvfrom(socket_fd, (void *) in_buf, MESSAGE_SIZE, 0, (struct sockaddr *) &remote_addr, &addr_len);
    if (n<0){
        exit(1);
    }
    cout<<"收到消息"<<in_buf<<endl;



    std::string msg=inet_ntoa(remote_addr.sin_addr);
    msg+=":"+std::to_string(htons(remote_addr.sin_port));
    sendMsgTo(socket_fd,remote_addr,msg);


}
//可以理解成处理请求，删除断开的客户端
void check_sockets(IO_Set &ioSet) {

    for (int i = 0; i <= FD_SIZE ; ++i) {
        int clientfd = ioSet.client_fds[i];

        if (clientfd != -1 && FD_ISSET(clientfd, &ioSet.fd_ready_sets)) {
             echo(clientfd);
        }
    }


}


//udp服务器
//与tcp最大的不同就是不用建立连接，不用监听端口
//客户端随时可以发送数据包给服务器，不论是否开启服务器

//nc -u 127.0.0.1 8888
int main(int argc,char* argv[]) {

    if (argc<3){
        cout<<"Usage: localport1 localport2"<<endl;
        return 0;
    }

    IO_Set pool;
    init_io_set(pool);
    int socket_fd= createUDPSocket("0.0.0.0", atoi(argv[1]));
    int socket_fd2= createUDPSocket("0.0.0.0", atoi(argv[2]));

    add_socket(pool,socket_fd);
    add_socket(pool,socket_fd2);

    //////////////////////////////接受数据///////////////////////////
    while (true) {
        ////////////////设置监听的IO集///////////////
        pool.fd_ready_sets = pool.fd_sets;
        //////////////////////////////////////////////////////

        select(pool.max_fd + 1, &pool.fd_ready_sets, NULL, NULL, 0);

        check_sockets(pool);
    }


    close(socket_fd);


    return 0;
}

