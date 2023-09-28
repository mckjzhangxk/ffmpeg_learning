#include <event2/listener.h>
#include <event.h>
#include <sys/socket.h>
/*
 * 知名的异步IO库
 * libuv:nodejs
   libev
   libevthp:http库，基于libevent
   libevent
 * */
void on_read(struct bufferevent *bev, void *ctx) {
    //- 内部由 输入，输出缓冲区组成
    evbuffer *input = bufferevent_get_input(bev);
    evbuffer *output = bufferevent_get_output(bev);
    //echo 功能
    evbuffer_add_buffer(output,input);
}

//bufferevent的作用？
//- 外部看缓冲区，可以与socket绑定
//- 内部由 输入，输出缓冲区组成
//- 每个socket对呀一个bufferevent
//- socket有事件触发，可以设置callback
void bind_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *param) {

    struct event_base *base = evconnlistener_get_base(listener);
    //对socket绑定了缓冲区bufferevent
    struct bufferevent *bufferevent = bufferevent_socket_new(base, fd, 0);

    bufferevent_enable(bufferevent, EV_READ | EV_WRITE);
    bufferevent_setcb(bufferevent, on_read, NULL, NULL, NULL);
}

//otool -L tcp_server_libevent //mac 查看需要的动态库

//学习资料
//## [libevent API](https://libevent.org/)
//https://libevent.org/libevent-book/

int main() {
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(8888);

    struct event_base *base = event_base_new();//等价于epoll_create
    //创建了一直socket,
    //把socket加入epoll监控
    //执行回调event_callback
    struct evconnlistener *evconnlistener = evconnlistener_new_bind(base,
                                                                    bind_callback,
                                                                    NULL,
                                                                    LEV_OPT_REUSEABLE,
                                                                    10,
                                                                    (struct sockaddr *) &local_addr,
                                                                    sizeof(local_addr));
    event_base_dispatch(base);//等价于epoll_wait

    event_base_free(base);

}