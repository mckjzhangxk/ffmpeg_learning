#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>

#define FD_SIZE  256

int create_socket(int port) {
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }
    int on = 1;
    int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    return s;
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n"                  \
    "<script src='a.js' type='application/javascript'></script>\r\n" \
    "<script src='b.js' type='application/javascript'></script>\r\n" \
    "<script src='c.js' type='application/javascript'></script>\r\n" \
    "<script src='d.js' type='application/javascript'></script>\r\n" \
    "<script src='e.js' type='application/javascript'></script>\r\n" \
    "<script src='f.js' type='application/javascript'></script>\r\n" \
    "<script src='g.js' type='application/javascript'></script>\r\n" \
    "<script src='h.js' type='application/javascript'></script>\r\n" \
    "<script src='i.js' type='application/javascript'></script>\r\n" \
    "<script src='j.js' type='application/javascript'></script>\r\n" \
    "<script src='k.js' type='application/javascript'></script>\r\n" \
    "<script src='l.js' type='application/javascript'></script>\r\n" \
    "<script src='m.js' type='application/javascript'></script>\r\n" \
    "<script src='n.js' type='application/javascript'></script>\r\n" \
    "<script src='o.js' type='application/javascript'></script>\r\n" \
    "<script src='p.js' type='application/javascript'></script>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"

void configure_context(SSL_CTX *ctx) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "1557605_www.learningrtc.cn.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "1557605_www.learningrtc.cn.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}


////////////////
typedef struct {
    fd_set fd_ready_sets; //这里是给select使用，让select去修改
    fd_set fd_sets;  //这里是新增 与删除使用的set
    int max_fd;

    int server_fd;
    int client_fds[FD_SIZE];
    int client_num;
    pthread_mutex_t mutex;
} IO_Set;

void init_io_set(IO_Set *ioSet, int listenFd) {

    FD_ZERO(&ioSet->fd_sets);
    FD_SET(listenFd, &ioSet->fd_sets);

    ioSet->server_fd = ioSet->max_fd = listenFd;
    int i;
    for (i = 0; i < FD_SIZE; ++i) {
        ioSet->client_fds[i] = -1;
    }
    ioSet->client_num = 0;
}

int add_client(IO_Set *ioSet, int clientFd) {
    pthread_mutex_lock(&ioSet->mutex);
    int i;
    for (i = 0; i < FD_SIZE; ++i) {
        if (ioSet->client_fds[i] == -1) {
            //描述符集 更新
            ioSet->client_fds[i] = clientFd;
            //监听集 更新
            FD_SET(clientFd, &ioSet->fd_sets);
            if (clientFd > ioSet->max_fd) {
                ioSet->max_fd = clientFd;
            }
            ioSet->client_num++;
            pthread_mutex_unlock(&ioSet->mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&ioSet->mutex);
    return -1;
}


void remove_client(IO_Set *ioSet, int clientFd) {
    pthread_mutex_lock(&ioSet->mutex);
    printf("remove fd %d\n", clientFd);

    FD_CLR(clientFd, &ioSet->fd_sets);
    ioSet->max_fd = ioSet->server_fd;
    int i;
    for (i = 0; i < FD_SIZE; ++i) {
        if (ioSet->client_fds[i] > ioSet->max_fd) {
            ioSet->max_fd = ioSet->client_fds[i];
        }
    }
//    close(clientFd);
    pthread_mutex_unlock(&ioSet->mutex);
}

//可以理解成处理请求，删除断开的客户端
int check_clients(IO_Set *ioSet) {
    pthread_mutex_lock(&ioSet->mutex);
    if (FD_ISSET(ioSet->server_fd, &ioSet->fd_ready_sets)) {
        pthread_mutex_unlock(&ioSet->mutex);
        return ioSet->server_fd;
    }
    int i;
    for (i = 0; i < FD_SIZE; ++i) {
        int clientfd = ioSet->client_fds[i];

        if (clientfd != -1 && FD_ISSET(clientfd, &ioSet->fd_ready_sets)) {
            pthread_mutex_unlock(&ioSet->mutex);
            return clientfd;
        }
    }
    pthread_mutex_unlock(&ioSet->mutex);
    return -1;

}

/////////////////http//////////////////////////
void set_fd_noblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void set_fd_block(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag & ~O_NONBLOCK);
}

int isBlock(int fd) {
    int flag = fcntl(fd, F_GETFL);

    return 0 == (O_NONBLOCK & flag);
}


//////////////

SSL *SSL_CONTEXTS[FD_SIZE];
//char SSL_ACCEPT[FD_SIZE];
pthread_t threads[FD_SIZE];

IO_Set io_rd_set;

void *thread_ssl_handshake(void *args) {
    int fd = (int) args;

    SSL *ssl = SSL_CONTEXTS[fd];
//    SSL_ACCEPT[fd] = 0;

    int ret;
    while (((ret = SSL_accept(ssl)) <= 0)) {
        int code = SSL_get_error(ssl, ret);
//        ERR_print_errors_fp(stderr);

        if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
            printf("handshake error [%d]\n",fd);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(fd);
            pthread_exit(NULL);
        }
    }

//    SSL_ACCEPT[fd] = 1;
    add_client(&io_rd_set,fd);
    printf("handshake success [%d]\n",fd);
    pthread_exit(NULL);


}


int main(int argc, char **argv) {

    int sock;
    SSL_CTX *ctx;
//    memset(SSL_ACCEPT, 0, FD_SIZE);
    /* Ignore broken pipe signals */
    signal(SIGPIPE, SIG_IGN);

    ctx = create_context();

    configure_context(ctx);

    sock = create_socket(8916);

    /* Handle connections */


    set_fd_noblock(sock);
    init_io_set(&io_rd_set, sock);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    while (1) {
        io_rd_set.fd_ready_sets = io_rd_set.fd_sets;
        int nReady = select(io_rd_set.max_fd + 1, &io_rd_set.fd_ready_sets, NULL, NULL, &timeout);

        if (nReady > 0) {
            int fd = check_clients(&io_rd_set);
            if (fd == sock) {
                struct sockaddr_in addr;
                unsigned int len = sizeof(addr);


                int client = accept(sock, (struct sockaddr *) &addr, &len);
                if (client < 0) {
                    perror("Unable to accept");
                    exit(EXIT_FAILURE);
                }
                printf("client[%d],port[%d],isBlock [%d]\n", client, ntohs(addr.sin_port), isBlock(client));

                SSL *ssl = SSL_new(ctx);
                SSL_set_fd(ssl, client);
//                int ret;
//                while (((ret = SSL_accept(ssl)) <= 0)) {
////                    printf("error\n");
//                    int code = SSL_get_error(ssl, ret);
//
//
//                    ERR_print_errors_fp(stderr);
//
//                    if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
//                        printf("handshake error\n");
//                        SSL_shutdown(ssl);
//                        SSL_free(ssl);
//                        close(client);
//                        goto repeat;
//                    }
//
//                }

                set_fd_block(client);
//                SSL_ACCEPT[fd] = 0;
//                add_client(&io_rd_set, client);
                SSL_CONTEXTS[client] = ssl;
                pthread_create(&threads[fd], NULL, thread_ssl_handshake, (void *)client);

            } else {
                SSL *ssl = SSL_CONTEXTS[fd];
                /*
                repeat:
                if (SSL_ACCEPT[fd] == 0) {
                    int ret = SSL_accept(ssl);
                    if (ret <= 0) {
                        int code = SSL_get_error(ssl, ret);
                        if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
                            remove_client(&io_rd_set, fd);
                            SSL_shutdown(ssl);
                            SSL_free(ssl);
                            close(fd);
                            SSL_ACCEPT[fd] = 0;
                            printf("handshake error [%d]\n", fd);
                        } else if (code == SSL_ERROR_WANT_WRITE) {
                            goto repeat;
                        }
                    } else {
                        SSL_ACCEPT[fd] = 1;
                        printf("handshake success [%d]\n", fd);
                    }

                    continue;
                }*/


                char buf[10240];
                int n = SSL_read(ssl, buf, sizeof(buf));
                printf("read [%d]->%d\n", fd, n);
                if (n <= 0) {
                    remove_client(&io_rd_set, fd);
                    SSL_shutdown(ssl);
                    SSL_free(ssl);
                    close(fd);
                    continue;
                }
                n = SSL_write(ssl, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
                printf("write [%d]->%d\n", fd, n);
                remove_client(&io_rd_set, fd);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(fd);
//                SSL_ACCEPT[fd] = 0;
            }
        } else {
            if (nReady<0)
            perror(strerror(errno));
        }
    }
    return 0;
//        struct sockaddr_in addr;
//        unsigned int len = sizeof(addr);
//        SSL *ssl;
//        const char reply[] = "test\n";
//
//        int client = accept(sock, (struct sockaddr*)&addr, &len);
//        if (client < 0) {
//            perror("Unable to accept");
//            exit(EXIT_FAILURE);
//        }
//        printf("hhh %d\n",client);
//        ssl = SSL_new(ctx);
//        SSL_set_fd(ssl, client);
//
//        if (SSL_accept(ssl) <= 0) {
//            printf("error\n");
//            ERR_print_errors_fp(stderr);
//        } else {
//            printf("write\n");
//            char buf[1024];
//            SSL_read(ssl,buf,sizeof(buf));
////            printf("%s\n",buf);
//            SSL_write(ssl, HTTP_RESPONSE, strlen(HTTP_RESPONSE));
//        }
//
//        SSL_shutdown(ssl);
//        SSL_free(ssl);
//        close(client);
//    }
//
//    close(sock);
//    SSL_CTX_free(ctx);
}
