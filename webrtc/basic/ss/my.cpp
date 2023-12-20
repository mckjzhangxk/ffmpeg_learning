#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#define FD_SIZE 2
#define MESSAGE_SIZE 1024
int open_listen_fd() {

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
    local_addr.sin_port = htons(9898);
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

void set_fd_noblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

struct IO_Set {
    fd_set fd_ready_sets; //这里是给select使用，让select去修改
    fd_set fd_sets;  //这里是新增 与删除使用的set

    int nReady;  //多少fd准备好，至少有1个字节的数据

    int max_pos;  //accept_fds中最大有效位置
    int max_fd;

    int client_fds[FD_SIZE];
    int client_num;
};

void init_io_set(IO_Set &ioSet) {

    FD_ZERO(&ioSet.fd_sets);

    ioSet.max_pos = -1;

    for (int i = 0; i < FD_SIZE; ++i) {
        ioSet.client_fds[i] = -1;
    }
    ioSet.client_num = 0;
}

int add_client(IO_Set &ioSet, int clientFd) {
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
    return -1;

}

void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n", sig);
}


void replaceCRLF(char *str) {
    // 获取字符串的长度
    size_t len = strlen(str);

    // 遍历字符串
    for (size_t i = 0; i < len - 1; i++) {
        // 检查是否是\r\n
        if (str[i] == '\r' && str[i + 1] == '\n') {
            // 替换为\n
            str[i] = '\n';

            // 移动后续字符
            for (size_t j = i + 1; j < len; j++) {
                str[j] = str[j + 1];
            }

            // 更新字符串长度
            len--;

            // 由于我们删除了一个字符，所以需要重新检查当前位置
            i--;
        }
    }
}
int main(int argc,char* argv[]) {

    char line[]="hello world zxk ";
    int index= strlen(line)+1;
    char * token=line;
     argc=1; //number of \0
    while ( *token && (token=strchr(token,' ')) )
    {
        *token=0;
        token+=1;
        argc+=1;
    }
    argv=(char**)malloc(sizeof(argc+1));
    argv[0]=0;
    argv[1]=line;
    int c=2;
    for (int i = 0; i < index-1; i++)
    {
        if(line[i]==0){
            argv[c++]=line+i+1;

        }
    }


    int socket_fd = open_listen_fd();
    while (true) {
        socklen_t addr_len = sizeof(struct sockaddr_in);

        //accept an new connection, block......
        struct sockaddr_in remote_addr;

        int accept_fd = accept(socket_fd, (struct sockaddr *) &remote_addr, &addr_len);
        if (accept_fd < 0) {
            continue;
        }

        /////////////fork一个shell进程/////////////////
        int child_stdin_fd[2];
        if (pipe(child_stdin_fd) == -1) {
            return -1;
        }
        int child_stdout_fd[2];
        if (pipe(child_stdout_fd) == -1) {
            return -1;
        }
        set_fd_noblock(child_stdout_fd[0]);
        set_fd_noblock(child_stdout_fd[1]);
        set_fd_noblock(child_stdin_fd[1]);


        pid_t pid = fork();
        if (pid < 0) {
            return -1;
        }

        if (pid == 0) {
            //stdin
            close(child_stdin_fd[1]);
            dup2(child_stdin_fd[0], 0);

            //stout stderr
            close(child_stdout_fd[0]);
            dup2(child_stdout_fd[1], 1);
            dup2(child_stdout_fd[1], 2);


            const char *argv[] = {"/bin/sh", NULL};
            char *environ[] = {NULL};

            char *shell = "/bin/sh";

            int r = execve(shell, (char **) argv, environ);
            if (r < 0) {
                printf("fail to exec command\n");
            }

        } else {
            close(child_stdin_fd[0]);
            close(child_stdout_fd[1]);


            IO_Set ioSet;
            init_io_set(ioSet);
            add_client(ioSet, child_stdout_fd[0]);
            add_client(ioSet, accept_fd);

            while (true) {
                ioSet.fd_ready_sets = ioSet.fd_sets;
//            printf("最大fd %d,客户端数 %d\n", ioSet.max_fd, ioSet.client_num);
                //////////////////////////////////////////////////////
                ioSet.nReady = select(ioSet.max_fd + 1, &ioSet.fd_ready_sets, NULL, NULL, 0);

                if (ioSet.nReady < 0) {
                    perror("select error");
                    break;
                } else if (ioSet.nReady == 0) {
                    printf("select time out ......");
                    continue;
                } else if (FD_ISSET(child_stdout_fd[0], &ioSet.fd_ready_sets)) {//child has output
                    int status;
                    pid = waitpid(-1, &status, WNOHANG);

                    if (pid > 0) {
                        printf("child %d exit,status %d\n", pid, status);
                        close(accept_fd);
                        break;
                    } else if (pid < 0) {
                        printf("child %d waitpid error\n", pid);
                        close(accept_fd);
                        break;
                    }
                    char buffer[128];
                    int n = 0;
                    while ((n = read(child_stdout_fd[0], buffer, sizeof(buffer))) > 0) {
                        write(accept_fd,buffer,n);
                    }
                } else if(FD_ISSET(accept_fd, &ioSet.fd_ready_sets)){
                    char shell_cmd[MESSAGE_SIZE] = {0,};
                    int n = recv(accept_fd , &shell_cmd, MESSAGE_SIZE, 0);
                    if(n == 0){
                        printf("EOF\n");
                        break;
                    }
                    replaceCRLF(shell_cmd);
                    write(child_stdin_fd[1], shell_cmd, strlen(shell_cmd));

                }

            }


        }
    }


}