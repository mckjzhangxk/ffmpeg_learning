#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include "rtp.h"

#define RTP_PAYLOAD_MAX_SIZE        1400
#define SEND_BUF_SIZE               1500
#define MESSAGE_SIZE 1600
#define  RTP_HEADER_SIZE 12

#define FD_SIZE 2
#define NAL_BUF_SIZE                1500 * 250
#define FILENAME "/Users/zhanggxk/project/h264_to_rtp/tvt.h264"
uint8_t nal_buf[NAL_BUF_SIZE];
void sigint_handler(int sig) /* SIGINT handler */
{

    printf("捕捉到信号 %d\n", sig);

}

// 1-start 2-end,0
void init_fragment_header(fu_indicator_t *fu_ind, fu_header_t *fu_hdr, uint8_t nalu_hdr, int frag_type = 0) {
    fu_ind->f = (nalu_hdr & 0x80) >> 7;
    fu_ind->nri = (nalu_hdr & 0x60) >> 5;
    fu_ind->type = 28;


    fu_hdr->s = 0;
    fu_hdr->e = 0;
    fu_hdr->r = 0;
    fu_hdr->type = nalu_hdr & 0x1f;

    switch (frag_type) {
        case 1:
            fu_hdr->s = 1;
            break;
        case 2:
            fu_hdr->e = 1;
            break;
        default:
            break;
    }
}

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


class IO_Set {

public:
    IO_Set() : m_nReady(0), m_max_fd(-1), m_client_num(0) {
        FD_ZERO(&m_fd_sets);


        for (int i = 0; i < FD_SIZE; ++i) {
            m_client_fds[i] = -1;
        }

    }

    int add_client(int clientFd) {
        for (int i = 0; i < FD_SIZE; ++i) {
            if (m_client_fds[i] == -1) {
                //描述符集 更新
                m_client_fds[i] = clientFd;

                //监听集 更新
                FD_SET(clientFd, &m_fd_sets);
                if (clientFd > m_max_fd) {
                    m_max_fd = clientFd;
                }

                m_client_num++;
                return 0;
            }
        }
        printf("无法服务更多客户端,当前 %d个客户端 \n", m_client_num);
        return -1;

    }

    bool check_client(int fd) {
        return FD_ISSET(fd, &m_fd_ready_sets);
    }

    void wait_incoming_packet() {
        m_fd_ready_sets = m_fd_sets;
        m_nReady = select(m_max_fd + 1, &m_fd_ready_sets, NULL, NULL, 0);
    }

private:
    fd_set m_fd_ready_sets; //这里是给select使用，让select去修改
    fd_set m_fd_sets;  //这里是新增 与删除使用的set
    int m_nReady;  //多少fd准备好，至少有1个字节的数据
    int m_max_fd;
    int m_client_fds[FD_SIZE];
    int m_client_num;
};

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

class RTSP{
public:
    RTSP(int fd):sockFd(fd){

    }

    int  Send(void *buffer,int size){
        int n=send(sockFd,buffer,size,0);
        return n;
    }
    int sockFd;
};


void  init_rtsp_header(rtp_header_t *rtp_hdr,uint16_t& seq_num,uint32_t ts_current,int ssrc,int marker=0){

    /*
     * 1. 设置 rtp 头
     */
    rtp_hdr->csrc_len = 0;
    rtp_hdr->extension = 0;
    rtp_hdr->padding = 0;
    rtp_hdr->version = 2;
    rtp_hdr->payload_type = H264;
    rtp_hdr->marker = marker;
    printf("seq %d\n",seq_num);
    rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
    rtp_hdr->timestamp = htonl(ts_current);
    rtp_hdr->ssrc = htonl(ssrc);

}
static int h264nal2rtp_send(int framerate, uint8_t *nalu_buf, int nalu_len,rtp_header_t* init_rtp_header,RTSP & rtsp,int init)
{
    static uint32_t ts_current = init_rtp_header->timestamp;
    static uint16_t seq_num = init_rtp_header->seq_no;

    if (init==1){
        ts_current = init_rtp_header->timestamp;
        seq_num = init_rtp_header->seq_no;
    }
    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */


    /*
     * 加入长度判断，
     * 当 nalu_len == 0 时， 必须跳到下一轮循环
     * nalu_len == 0 时， 若不跳出会发生段错误!
     * fix by hmg
     */
    if (nalu_len < 1) {
        return 0;
    }
    rtp_packet_t packet;
    packet.interleaved[0] = '$';  // 与rtsp区分
    packet.interleaved[1] = 0;    // 区别rtp和rtcp。0-1
    uint8_t* SENDBUFFER=packet.rtp_packet;

    rtp_header_t *rtp_hdr=(rtp_header_t*)SENDBUFFER;





    if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {
        /*
         * single nal unit
         */

        memset(SENDBUFFER, 0, SEND_BUF_SIZE);

        //1. 设置 rtp 头
        init_rtsp_header(rtp_hdr,seq_num,ts_current,init_rtp_header->ssrc);
        //2. 填充nal内容
        memcpy(SENDBUFFER + RTP_HEADER_SIZE, nalu_buf, nalu_len );    /* 不拷贝nalu头 */

        //3. 发送打包好的rtp到客户端

        // rtp包载荷大小
        int rtp_size=RTP_HEADER_SIZE+nalu_len;
        packet.interleaved[2] = (rtp_size & 0xff00)>>8;
        packet.interleaved[3] = rtp_size & 0xff;

        if(rtsp.Send((void *)&packet, 4+RTP_HEADER_SIZE + nalu_len)<=0){
            return -1;
        }

    } else {
        //FU-A分割

        int nalu_len_minus_one=nalu_len-1; //去掉nalu头部
        uint8_t *payload_buffer=nalu_buf + 1;

        /* nalu 需要分片发送时分割的个数 */
        int fu_pack_num = nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? (nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE;
        /* 最后一个分片的大小 */
        int last_fu_pack_size = nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;


        for (int fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
            memset(SENDBUFFER, 0, SEND_BUF_SIZE);
            //1. 设置 rtp 头,12个字节
            init_rtsp_header(rtp_hdr,seq_num,ts_current,init_rtp_header->ssrc,fu_pack_num==fu_pack_num-1);

            //2. 设置 rtp 荷载头部,2字节
            fu_indicator_t * fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
            fu_header_t * fu_hdr = (fu_header_t *)&SENDBUFFER[13];
            uint8_t nalu_header=nalu_buf[0];
            int flag_type=fu_seq == 0?1:fu_seq ==fu_pack_num - 1?2:0;
            init_fragment_header(fu_ind,fu_hdr,nalu_header,flag_type);

            int data_size=fu_seq ==fu_pack_num - 1?last_fu_pack_size:RTP_PAYLOAD_MAX_SIZE;
            memcpy(SENDBUFFER + 14, payload_buffer+ RTP_PAYLOAD_MAX_SIZE * fu_seq, data_size );//3. 填充nalu内容
            size_t rtp_size = RTP_HEADER_SIZE + 2 + data_size;  /* rtp头 + nalu头 + nalu内容 */


            packet.interleaved[2] = (rtp_size & 0xff00)>>8;
            packet.interleaved[3] = rtp_size & 0xff;

            if (rtsp.Send((void *)&packet,4+rtp_size)<=0){
                return -1;
            }


        }

    }
    return 0;
}


class Inceptor {
    struct Fragment {
        int marker;
        char data[RTP_PAYLOAD_MAX_SIZE+4];
        int size;
        int type;
    };

    class Fragments {
    public:
        Fragments(RTSPSource *source) : m_source(source),m_pos(10000) {

        }

        Fragment getFragment() {
            if (m_pos >= m_fragments.size()) {
                m_fragments.clear();
                m_pos = 0;

                int nalu_len = 0;
                int ret = 0;
                uint8_t nalu_buf[NAL_BUF_SIZE];
                //read one valid packet
                while (true) {
                    ret = m_source->copy_nal_from_file(nalu_buf, &nalu_len);
                    if (ret > 0)
                        break;
                    else if (ret < 0) {
                        m_source->Reset();
                    }
                }

                //
                if (nalu_len < RTP_PAYLOAD_MAX_SIZE) {
                    Fragment fragment;
                    fragment.marker = 0;
                    fragment.size = nalu_len;
                    fragment.type=1;
                    memcpy(fragment.data, nalu_buf, nalu_len);
                    m_fragments.emplace_back(fragment);
                } else {
                    int nalu_len_minus_one = nalu_len - 1; //去掉nalu头部
                    uint8_t *payload_buffer = nalu_buf + 1;
                    /* nalu 需要分片发送时分割的个数 */
                    int fu_pack_num =
                            nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? (nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE + 1)
                                                                      : nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE;
                    /* 最后一个分片的大小 */
                    int last_fu_pack_size =
                            nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE
                                                                      : RTP_PAYLOAD_MAX_SIZE;

                    for (int fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
                        Fragment fragment;
                        fragment.type=2;

                        fu_indicator_t *fu_ind = (fu_indicator_t *) &fragment.data[0];
                        fu_header_t *fu_hdr = (fu_header_t *) &fragment.data[1];
                        uint8_t nalu_header = nalu_buf[0];

                        int flag_type = fu_seq == 0 ? 1 : fu_seq == fu_pack_num - 1 ? 2 : 0;
                        if (flag_type==2){
                            fragment.marker = 1;
                            fragment.size = last_fu_pack_size;
                        } else{
                            fragment.marker = 0;
                            fragment.size = RTP_PAYLOAD_MAX_SIZE;
                        }
                        init_fragment_header(fu_ind, fu_hdr, nalu_header, flag_type);

                        int data_size = fu_seq == fu_pack_num - 1 ? last_fu_pack_size : RTP_PAYLOAD_MAX_SIZE;

                        memcpy(&fragment.data[2], payload_buffer + RTP_PAYLOAD_MAX_SIZE * fu_seq, data_size);
                        m_fragments.emplace_back(fragment);
                    }
                }

            }

            return m_fragments[m_pos++];
        }

    private:
        RTSPSource *m_source;
        std::vector<Fragment> m_fragments;
        int m_pos;
    };

public:
    Inceptor() : m_flag(false), m_previous_status(false) {
        m_source = new RTSPSource(FILENAME);
        m_fragments = new Fragments(m_source);
    }

    ~Inceptor() {
        delete m_source;
    }

    void begin() {
        m_flag = true;
        m_previous_status = false;
    }

    void end() {
        m_flag = false;
        m_previous_status = false;
    }

    bool handle(int fd, char *buf, int n) {
        if (m_flag) {
            //rtp packet
            if ('$' == buf[0]) {

                //已经拦截成功了，后续可以继续拦截，否则必须等待一个新的开始包
                if (m_previous_status || canIncept(&buf[16])) {
                    m_previous_status = true;
                    //转发。。。
                    rtp_header_t *rtp_header = (rtp_header_t *) &buf[4];

                    //大端序->小端序
                    rtp_header->seq_no = ntohs(rtp_header->seq_no);
                    rtp_header->timestamp = ntohl(rtp_header->timestamp);
                    rtp_header->ssrc = ntohl(rtp_header->ssrc);

                    //不断转发

                    RTSPSource source_2(FILENAME);
                    int len;
                    int cnt=0;
                    int ret=0;

                    RTSP rtsp(fd);
                    while (m_flag){
                        printf("%d\n",cnt++);
                        ret=source_2.copy_nal_from_file(nal_buf, &len);
                        if (ret<0){
                            break;
                        }
                        h264nal2rtp_send(25, nal_buf, len, rtp_header,rtsp,cnt);

                        usleep(1000*33);
                    }

                    return true;
                } else {
                    goto DIRECT_SEND;
                }

            } else{
                goto DIRECT_SEND;
            }
        } else {
            goto DIRECT_SEND;
        }
        return true;
        DIRECT_SEND:
        //直接转发数据包
        if (!send_data(fd, buf, n)) {
            return false;
        } else {
            return true;
        }


    }

private:
    //只判断当前的数据包是否可以被劫持
    bool canIncept(char *nalu) {
        fu_indicator_t *fu_indicator = (fu_indicator_t *) &nalu;

        if (fu_indicator->type == 28) {
            fu_header_t *fu_header = (fu_header_t *) &nalu[1];
            fu_header = fu_header;
            if (fu_header->s) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    }

    bool write_rtp_packet(int fd, rtp_header_t *rtp_header) {

        Fragment fragment = m_fragments->getFragment();
//        Fragment fragment;
//        fragment.size=100;
//        memset(fragment.data,0,fragment.size);

        // rtp包载荷大小
        int rtp_size = RTP_HEADER_SIZE + fragment.size;

        rtp_packet_t packet;
        packet.interleaved[0] = '$';  // 与rtsp区分
        packet.interleaved[1] = 0;    // 区别rtp和rtcp。0-1
        packet.interleaved[2] = (rtp_size & 0xff00) >> 8;
        packet.interleaved[3] = rtp_size & 0xff;


        uint8_t *SENDBUFFER = packet.rtp_packet;

        //1. 设置 rtp 头
        rtp_header->marker = fragment.marker;
        memcpy(SENDBUFFER, rtp_header, RTP_HEADER_SIZE);
        //2. 填充nal内容
        memcpy(SENDBUFFER + RTP_HEADER_SIZE, fragment.data, fragment.size);    /* 不拷贝nalu头 */

        //3. 发送打包好的rtp到客户端

        printf("%d  %d\n",fragment.type,fragment.size);
        return send_data(fd, (char *) &packet, 4 + RTP_HEADER_SIZE + fragment.size);
    }

    bool m_flag;
    bool m_previous_status;
    RTSPSource *m_source;
    Fragments *m_fragments;

};

Inceptor g_interceptor;

void client_request_handler(int accept_fd) {
    char in_buf[MESSAGE_SIZE] = {0,};

    int serverfd = open_connect_fd("127.0.0.1", 7778);


    if (serverfd == -1) {
        printf("无法代理,服务器不通\n");
        close(accept_fd);
        return;
    }

    IO_Set ioSet;
    ioSet.add_client(serverfd);
    ioSet.add_client(accept_fd);


    while (true) {
        ioSet.wait_incoming_packet();
        memset(in_buf, 0, MESSAGE_SIZE);
        int n = 0;
        if (ioSet.check_client(serverfd)) {
            bool ret = receive_data(serverfd, in_buf, MESSAGE_SIZE, n);

            if (!ret) {
                break;
            }

            if (!g_interceptor.handle(accept_fd, in_buf, n)) {
                break;
            }

        }
        if (ioSet.check_client(accept_fd)) {
            bool ret = redirect_data(accept_fd, serverfd, in_buf, MESSAGE_SIZE, n);
            if (!ret) {
                break;
            }
        }
    }
    printf("客户端退出\n");
    close(accept_fd);
    close(serverfd);
}


void client_command_handler(int accept_fd) {
    char in_buf[MESSAGE_SIZE] = {0,};
    int n = 0;
    while (true) {
        if (receive_data(accept_fd, in_buf, MESSAGE_SIZE, n) <= 0) {
            break;
        }

        if (in_buf[0] == 's') {
            g_interceptor.begin();
        } else if (in_buf[0] == 'e') {
            g_interceptor.end();
        }
        printf("%s", in_buf);
    }
    close(accept_fd);
}

int AcceptConnection(int socket_fd) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    //accept an new connection, block......
    struct sockaddr_in remote_addr;
    int accept_fd = accept(socket_fd, (struct sockaddr *) &remote_addr, &addr_len);
    if (accept_fd < 0) {
        return -1;
    }
    set_fd_noblock(accept_fd);
    return accept_fd;
}
//fork方式带来的问题

//1)资源可能会被长期占用
//2)分配子进程开销大
int main() {
//    signal(SIGUSR1, sigint_handler);
    signal(SIGCONT, sigint_handler);
    signal(SIGPIPE, sigint_handler);

    int socket_fd = open_listen_fd(7777);  //proxy
    int socket_fd2 = open_listen_fd(7779);//command

    printf("pid %d\n", getpid());
    IO_Set ioSet;
    ioSet.add_client(socket_fd);
    ioSet.add_client(socket_fd2);

//    g_interceptor.begin();
    //////////////////////////////接受连接///////////////////////////
    while (true) {
        ioSet.wait_incoming_packet();
        //来自播放器的请求
        if (ioSet.check_client(socket_fd)) {
            int accept_fd = AcceptConnection(socket_fd);
            if (accept_fd < 0) {
                continue;
            }
            std::thread thread(client_request_handler, accept_fd);
            thread.detach();
        }


        if (ioSet.check_client(socket_fd2)) {
            int accept_fd = AcceptConnection(socket_fd2);
            if (accept_fd < 0) {
                continue;
            }
            client_command_handler(accept_fd);
        }


    }

    printf("服务器关闭 \n");
    close(socket_fd);
    close(socket_fd2);
}

