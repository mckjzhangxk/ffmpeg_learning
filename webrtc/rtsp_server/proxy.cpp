#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <thread>
#include <vector>
#include "rtp.h"

#define RTP_PAYLOAD_MAX_SIZE        1400
#define SEND_BUF_SIZE               1500
#define MESSAGE_SIZE 65536
#define  RTP_HEADER_SIZE 12

#define FD_SIZE 2
#define NAL_BUF_SIZE                1500 * 250
#define FILENAME "/Users/zhanggxk/Desktop/测试视频/my.h265"
#define RTSP_VIDEO_IP "114.34.141.233"
#define RSTP_INTERLEAVE_SIZE 4

extern bool receive_data(int recv_fd, char *in_buf, int in_buf_size, int &out_n);

extern bool send_data(int send_fd, char *in_buf, int in_n);

extern bool redirect_data(int recv_fd, int send_fd, char *in_buf, int in_buf_size, int &out_n);

extern void set_fd_noblock(int fd);

extern int open_listen_fd(int port);

extern int open_connect_fd(const char *remoteIp, int remotePort);

//uint8_t nal_buf[NAL_BUF_SIZE];
bool is265 = true;
bool g_inception = false;

void sigint_handler(int sig) /* SIGINT handler */
{

    if (sig == SIGUSR1) {
        g_inception = true;
    }
    if (sig == SIGUSR2) {
        g_inception = false;
    }
    printf("捕捉到信号 %d\n", sig);

}

// frag_type 1-start 2-end,0-非开始非结束
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

// frag_type 1-start 2-end,0-非开始非结束
void init_fragment_header(fu_indicator265_t *fu_ind, fu_header265_t *fu_hdr, uint8_t nalu_hdr, int frag_type = 0) {
    fu_ind->f = (nalu_hdr & 0x80) >> 7;
    fu_ind->type = 49;
    fu_ind->nuh_layer_id = nalu_hdr & 0x01;


    fu_hdr->s = 0;
    fu_hdr->e = 0;
    fu_hdr->type = (nalu_hdr & 0x7E) >> 1;

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

    bool wait_incoming_packet() {
        m_fd_ready_sets = m_fd_sets;
        m_nReady = select(m_max_fd + 1, &m_fd_ready_sets, NULL, NULL, 0);
        if (m_nReady < 0) {
            return false;
        }
        return true;
    }

private:
    fd_set m_fd_ready_sets; //这里是给select使用，让select去修改
    fd_set m_fd_sets;  //这里是新增 与删除使用的set
    int m_nReady;  //多少fd准备好，至少有1个字节的数据
    int m_max_fd;
    int m_client_fds[FD_SIZE];
    int m_client_num;
};




//class RTSP {
//public:
//    RTSP(int fd) : sockFd(fd) {
//
//    }
//
//    int Send(void *buffer, int size) {
//        int n = send(sockFd, buffer, size, 0);
//        return n;
//    }
//
//    int sockFd;
//};


//void init_rtsp_header(rtp_header_t *rtp_hdr, uint16_t &seq_num, uint32_t ts_current, int ssrc, int marker = 0) {
//
//    /*
//     * 1. 设置 rtp 头
//     */
//    rtp_hdr->csrc_len = 0;
//    rtp_hdr->extension = 0;
//    rtp_hdr->padding = 0;
//    rtp_hdr->version = 2;
//    rtp_hdr->payload_type = H264;
//    if (is265)
//        rtp_hdr->payload_type = H265;
//    rtp_hdr->marker = marker;
//    printf("seq %d\n", seq_num);
//    rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
//    rtp_hdr->timestamp = htonl(ts_current);
//    rtp_hdr->ssrc = htonl(ssrc);
//
//}

//static int
//h264nal2rtp_send(int framerate, uint8_t *nalu_buf, int nalu_len, rtp_header_t *init_rtp_header, RTSP &rtsp, int init,
//                 bool is265 = false) {
//    static uint32_t ts_current = init_rtp_header->timestamp;
//    static uint16_t seq_num = init_rtp_header->seq_no;
//
//    if (init == 1) {
//        ts_current = init_rtp_header->timestamp;
//        seq_num = init_rtp_header->seq_no;
//    }
//    ts_current += (90000 / framerate);  /* 90000 / 25 = 3600 */
//
//
//    /*
//     * 加入长度判断，
//     * 当 nalu_len == 0 时， 必须跳到下一轮循环
//     * nalu_len == 0 时， 若不跳出会发生段错误!
//     * fix by hmg
//     */
//    if (nalu_len < 1) {
//        return 0;
//    }
//    rtp_packet_t packet;
//    packet.interleaved[0] = '$';  // 与rtsp区分
//    packet.interleaved[1] = 0;    // 区别rtp和rtcp。0-1
//    uint8_t *SENDBUFFER = packet.rtp_packet;
//
//    rtp_header_t *rtp_hdr = (rtp_header_t *) SENDBUFFER;
//
//
//    if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {
//        /*
//         * single nal unit
//         */
//
//        memset(SENDBUFFER, 0, SEND_BUF_SIZE);
//
//        //1. 设置 rtp 头
//        init_rtsp_header(rtp_hdr, seq_num, ts_current, init_rtp_header->ssrc);
//        //2. 填充nal内容
//        memcpy(SENDBUFFER + RTP_HEADER_SIZE, nalu_buf, nalu_len);    /* 不拷贝nalu头 */
//
//        //3. 发送打包好的rtp到客户端
//
//        // rtp包载荷大小
//        int rtp_size = RTP_HEADER_SIZE + nalu_len;
//        packet.interleaved[2] = (rtp_size & 0xff00) >> 8;
//        packet.interleaved[3] = rtp_size & 0xff;
//
//        if (rtsp.Send((void *) &packet, 4 + RTP_HEADER_SIZE + nalu_len) <= 0) {
//            return -1;
//        }
//
//    } else {
//        //FU-A分割
//
//        int nalu_len_minus_one = nalu_len - 1; //去掉nalu头部
//        uint8_t *payload_buffer = nalu_buf + 1;
//
//        if (is265) {
//            nalu_len_minus_one = nalu_len_minus_one - 1;
//            payload_buffer = payload_buffer + 1;
//        }
//        /* nalu 需要分片发送时分割的个数 */
//        int fu_pack_num = nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? (nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE + 1) :
//                          nalu_len_minus_one / RTP_PAYLOAD_MAX_SIZE;
//        /* 最后一个分片的大小 */
//        int last_fu_pack_size = nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE ? nalu_len_minus_one % RTP_PAYLOAD_MAX_SIZE
//                                                                          : RTP_PAYLOAD_MAX_SIZE;
//
//
//        for (int fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
//            memset(SENDBUFFER, 0, SEND_BUF_SIZE);
//            //1. 设置 rtp 头,12个字节
//            init_rtsp_header(rtp_hdr, seq_num, ts_current, init_rtp_header->ssrc, fu_pack_num == fu_pack_num - 1);
//
//            //2. 设置 rtp 荷载头部,2字节
//            int flag_type = fu_seq == 0 ? 1 : fu_seq == fu_pack_num - 1 ? 2 : 0;
//
//            if (is265) {
//                fu_indicator265_t *fu_ind = (fu_indicator265_t *) &SENDBUFFER[RTP_HEADER_SIZE];
//                SENDBUFFER[13] = nalu_buf[1];
//                fu_header265_t *fu_hdr = (fu_header265_t *) &SENDBUFFER[RTP_HEADER_SIZE + 2];
//
//                uint8_t nalu_header = nalu_buf[0];
//                init_fragment_header(fu_ind, fu_hdr, nalu_header, flag_type);
//            } else {
//                fu_indicator_t *fu_ind = (fu_indicator_t *) &SENDBUFFER[RTP_HEADER_SIZE];
//                fu_header_t *fu_hdr = (fu_header_t *) &SENDBUFFER[RTP_HEADER_SIZE + 1];
//                uint8_t nalu_header = nalu_buf[0];
//                init_fragment_header(fu_ind, fu_hdr, nalu_header, flag_type);
//            }
//
//
//            int data_size = fu_seq == fu_pack_num - 1 ? last_fu_pack_size : RTP_PAYLOAD_MAX_SIZE;
//            size_t rtp_size = 0;
//            if (is265) {
//                memcpy(SENDBUFFER + RTP_HEADER_SIZE + 3, payload_buffer + RTP_PAYLOAD_MAX_SIZE * fu_seq,
//                       data_size);//3. 填充nalu内容
//                rtp_size = RTP_HEADER_SIZE + 3 + data_size;  /* rtp头 + nalu头 + nalu内容 */
//            } else {
//                memcpy(SENDBUFFER + RTP_HEADER_SIZE + 2, payload_buffer + RTP_PAYLOAD_MAX_SIZE * fu_seq,
//                       data_size);//3. 填充nalu内容
//                rtp_size = RTP_HEADER_SIZE + 2 + data_size;  /* rtp头 + nalu头 + nalu内容 */
//            }
//
//
//            packet.interleaved[2] = (rtp_size & 0xff00) >> 8;
//            packet.interleaved[3] = rtp_size & 0xff;
//
//            if (rtsp.Send((void *) &packet, 4 + rtp_size) <= 0) {
//                return -1;
//            }
//
//
//        }
//
//    }
//    return 0;
//}


class Inceptor {
    struct Fragment {
        int marker;
        char data[RTP_PAYLOAD_MAX_SIZE + 4];
        int size;
    };

    class Fragments {
    public:
        Fragments(RTSPSource *source) : m_source(source), m_pos(10000) {

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
                    printf("ret %d\n", ret);
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
                    memcpy(fragment.data, nalu_buf, nalu_len);
                    m_fragments.emplace_back(fragment);
                } else {
                    int nalu_len_minus_one = nalu_len - 1; //去掉nalu头部
                    uint8_t *payload_buffer = nalu_buf + 1;
                    if (is265) {
                        nalu_len_minus_one = nalu_len_minus_one - 1;
                        payload_buffer = payload_buffer + 1;
                    }

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



                        //2. 设置 rtp 荷载头部,2字节
                        int flag_type = fu_seq == 0 ? 1 : fu_seq == fu_pack_num - 1 ? 2 : 0;

                        if (is265) {
                            fu_indicator265_t *fu_ind = (fu_indicator265_t *) &fragment.data[0];
                            fragment.data[1] = nalu_buf[1];
                            fu_header265_t *fu_hdr = (fu_header265_t *) &fragment.data[2];

                            uint8_t nalu_header = nalu_buf[0];
                            init_fragment_header(fu_ind, fu_hdr, nalu_header, flag_type);
                        } else {
                            fu_indicator_t *fu_ind = (fu_indicator_t *) &fragment.data[0];;
                            fu_header_t *fu_hdr = (fu_header_t *) &fragment.data[1];;
                            uint8_t nalu_header = nalu_buf[0];
                            init_fragment_header(fu_ind, fu_hdr, nalu_header, flag_type);
                        }

                        if (flag_type == 2) {
                            fragment.marker = 1;
                            fragment.size = last_fu_pack_size + 2;
                        } else {
                            fragment.marker = 0;
                            fragment.size = RTP_PAYLOAD_MAX_SIZE + 2;
                        }
                        if (is265) {
                            fragment.size += 1;
                        }


                        int data_size = fu_seq == fu_pack_num - 1 ? last_fu_pack_size : RTP_PAYLOAD_MAX_SIZE;

                        if (is265) {
                            memcpy(&fragment.data[3], payload_buffer + RTP_PAYLOAD_MAX_SIZE * fu_seq, data_size);
                        } else {
                            memcpy(&fragment.data[2], payload_buffer + RTP_PAYLOAD_MAX_SIZE * fu_seq, data_size);
                        }

                        m_fragments.emplace_back(fragment);
                    }
                }

            }

            return m_fragments[m_pos++];
        }

        void Reset() {
            m_fragments.clear();
            m_pos = 0;
            m_source->Reset();
        }

    private:
        RTSPSource *m_source;
        std::vector<Fragment> m_fragments;
        int m_pos;
    };

    class RTSPReader {
    public:
        RTSPReader() : position(-1), prepend(0) {}

        int read(char *buf, int n) {
            static int packet=0;
            char * buff=buf;
            //第一次没有初始化
            if (position == -1) {
                if (buf[0] == '$') {
                    position = 0;
                } else {
                    return 0;
                }
            }
            char origin_buf[prepend + n];
            memcpy(origin_buf, position_buf, prepend);
            memcpy(origin_buf + prepend, buf, n);

            if (prepend) {
                n+=prepend;
                prepend = 0;
            }

            if (position >= n) {
                position -= n;
                return 0;
            }


            while (position + 3 < n) {
                buf = &origin_buf[position];
                bool exit= false;
                if(buf[0]!='$'){
                    int reamined=n-position;

                    for (int i = 1; i <reamined ; ++i) {
                        buf+=1;
                        position+=1;
                        if (buf[0]=='$'){
                            break;
                        }
                    }
                    if (position+3<n){
                        buf = &origin_buf[position];
                    } else{
                        exit= true;
                    }

                }

                if (exit){
                    break;
                }

                short rtp_size = ntohs(*(short *) &buf[2]);
                if (rtp_size < 0) {
                    position=0;
                    return -1;
                }
                printRtpInfo(buf,n,packet);
                position += rtp_size + RSTP_INTERLEAVE_SIZE;
            }
            if (position >= n) {
                position -= n;
            } else {//这里没有读取到rtp_size,所以没法计算下次进入后的position
                for (int i = position; i < n; ++i) {
                    position_buf[i - position] = origin_buf[i];
                    prepend++;
                }
                position = 0;
            }

            packet++;
            return 0;
        }

        void printRtpInfo(char *buf,int n,int p) {
            short rtp_size = ntohs(*(short *) &buf[2]);

            rtp_header_t *rtpHeader = (rtp_header_t *) (buf + RSTP_INTERLEAVE_SIZE);

            printf("%d  positon: %8d seq %5d,PT %d,Size %d,recv size %d\n", p,position, ntohs(rtpHeader->seq_no), rtpHeader->payload_type,
                   rtp_size,n);
            if (rtpHeader->payload_type!=98) {
                return;
            }
        }

    private:
        long position;
        char position_buf[4];
        int prepend;
    };

public:
    Inceptor() : m_previous_status(false) {
        m_source = new RTSPSource(FILENAME);
        m_fragments = new Fragments(m_source);
    }

    ~Inceptor() {
        delete m_source;
    }

    bool isRtpPacket(char *buf, int n) {
        if (n < 6)
            return false;
        rtp_header_t *rtp_header = (rtp_header_t *) &buf[4];

//        bool ret= ('$' == buf[0])&&(0==buf[1])&&(rtp_header->version==2)&&(rtp_header->padding==0)&&(rtp_header->extension==0)&&(rtp_header->csrc_len==0)&&(rtp_header->payload_type==H264);
        bool ret = '$' == buf[0];
        if (ret) {
            short *s = (short *) &buf[2];

            short rtp_size = ntohs(*s);
//            printf("rtp_size %d\n",rtp_size);
        }
        return ret;
    }

    //截获buf,大小n
    //fd是要发生数据的一端
    bool handle(int fd, char *buf, int n) {
        int result=m_rtp_reader.read(buf, n);

        if (g_inception) {
            if (m_previous_status == false && (isRtpPacket(buf, n) && canIncept(&buf[4 + RTP_HEADER_SIZE]))) {
                m_previous_status = true;
                m_rtp_header = *((rtp_header_t *) &buf[4]);
                //大端序->小端序
                m_rtp_header.seq_no = ntohs(m_rtp_header.seq_no);
                m_rtp_header.timestamp = ntohl(m_rtp_header.timestamp);
                m_rtp_header.ssrc = ntohl(m_rtp_header.ssrc);
                m_fragments->Reset();
            } else if (m_previous_status) {
                m_rtp_header.seq_no++;
                m_rtp_header.timestamp += 90000 / 30;
            }
            //rtp packet
            if (m_previous_status) {
                return write_rtp_packet(fd, m_rtp_header);
            } else {
                printf("here\n");
                goto DIRECT_SEND;
            }
        } else {
            m_previous_status = false;
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

    bool write_rtp_packet(int fd, rtp_header_t rtp_header) {

        rtp_header.seq_no = htons(rtp_header.seq_no);
        rtp_header.timestamp = htonl(rtp_header.timestamp);
        rtp_header.ssrc = htonl(rtp_header.ssrc);


        Fragment fragment = m_fragments->getFragment();

        // rtp包载荷大小
        int rtp_size = RTP_HEADER_SIZE + fragment.size;


        rtp_packet_t packet;
        packet.interleaved[0] = '$';  // 与rtsp区分
        packet.interleaved[1] = 0;    // 区别rtp和rtcp。0-1
        packet.interleaved[2] = (rtp_size & 0xff00) >> 8;
        packet.interleaved[3] = rtp_size & 0xff;


        uint8_t *SENDBUFFER = packet.rtp_packet;

        //1. 设置 rtp 头
        rtp_header.marker = fragment.marker;
//        rtp_header->timestamp
        memcpy(SENDBUFFER, &rtp_header, RTP_HEADER_SIZE);
        //2. 填充nal内容
        memcpy(SENDBUFFER + RTP_HEADER_SIZE, fragment.data, fragment.size);    /* 不拷贝nalu头 */

        //3. 发送打包好的rtp到客户端

        return send_data(fd, (char *) &packet, 4 + rtp_size);
    }

    bool m_previous_status;//是否开始拦截
    RTSPSource *m_source;

    Fragments *m_fragments;

    rtp_header_t m_rtp_header;

    RTSPReader m_rtp_reader;
};


void client_request_handler(int accept_fd) {
    Inceptor g_interceptor;

    char *in_buf = (char *) malloc(MESSAGE_SIZE);
    //这里是要hook的服务器

    int serverfd = open_connect_fd(RTSP_VIDEO_IP, 554);


    if (serverfd == -1) {
        printf("无法代理,服务器不通\n");
        close(accept_fd);
        return;
    }

    IO_Set ioSet;
    ioSet.add_client(serverfd);
    ioSet.add_client(accept_fd);


    while (true) {
        if (!ioSet.wait_incoming_packet()) {
            continue;
        }

        memset(in_buf, 0, MESSAGE_SIZE);
        int n = 0;
        //来自rtsp的响应
        if (ioSet.check_client(serverfd)) {
            bool ret = receive_data(serverfd, in_buf, MESSAGE_SIZE, n);

            if (!ret) {
                break;
            }

            if (!g_interceptor.handle(accept_fd, in_buf, n)) {
                break;
            }

        }
        if (ioSet.check_client(accept_fd)) {//客户端直接转发给服务器
            //转发给rtsp服务器
            bool ret = redirect_data(accept_fd, serverfd, in_buf, MESSAGE_SIZE, n);
            if (!ret) {
                break;
            }
        }
    }
    printf("客户端退出\n");
    free(in_buf);
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
            g_inception = true;
        } else if (in_buf[0] == 'e') {
            g_inception = false;
        }
        printf("%s", in_buf);
    }
    close(accept_fd);
}

//客户端的连接请求
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

int main() {
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
        bool r = ioSet.wait_incoming_packet();
        if (!r) continue;
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
            std::thread thread(client_command_handler, accept_fd);
            thread.detach();
        }
    }
}

