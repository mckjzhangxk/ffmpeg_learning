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
#define FILENAME "/Users/zhanggxk/ana/jojo_u8.pcm"
//#define RTSP_VIDEO_IP "114.34.141.233"
#define RTSP_VIDEO_IP "1.226.190.115"
#define RTSP_VIDEO_PORT 6036
#define RSTP_INTERLEAVE_SIZE 4

#define MAGIC_SIZE  ((RSTP_INTERLEAVE_SIZE+RTP_HEADER_SIZE+3-1))

enum FRAME_TYPE {
    VIDEO,
    VIDEO_IDR,
    AUDIO,
    NOT_INTEREST,
    TRUCK_HEADER,
    UNKNOWN,
    PACKETED
};

struct FrameInfo {
    FRAME_TYPE type;
    int length2;
    int length6;
    int length9;
    int payload_length;

    int packet_size;
    int packet_index;
    int key_frame_idx;
    int header_end;

};

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


class ReadBuffer {
public:
    ReadBuffer() : m_buf_size(65546), m_pos(0) {
        m_buf = (char *) malloc(m_buf_size);
    }

    ~ReadBuffer() {
        if (m_buf) {
            free(m_buf);
            m_buf = NULL;
        }
    }

    int append(char *data, int n) {
        if ((m_pos + n) >= m_buf_size) {
            m_buf = (char *) realloc(m_buf, m_buf_size + n * sizeof(char));
            m_buf_size += n;
        }
        if (!m_buf)
            return -1;
        memcpy(m_buf + m_pos, data, n);
        m_pos += n;
        return 0;
    }

    //把缓冲的全局字节移出来

    //输出
    //out_buf，out_len:存放移出的数据
    int shift(char **out_buf, int *out_len) {
        *out_len = m_pos;
        if (*out_len) {
            *out_buf = (char *) malloc(sizeof(char) * (*out_len));
            memcpy(*out_buf, m_buf, *out_len);
        } else {
            return 0;
        }

        m_pos = 0;

        return 0;
    }

    int peek(char **out_buf, int *out_len) {
        *out_len = m_pos;
        if (*out_len) {
            *out_buf = (char *) malloc(sizeof(char) * (*out_len));
            memcpy(*out_buf, m_buf, *out_len);
        } else {
            return 0;
        }
        return 0;
    }

private:
    char *m_buf;
    int m_buf_size;
    int m_pos;
};

class TVT_Frame {
public:
    TVT_Frame() : m_start_code_len(0), m_skip_bytes(0) {
        pcmSource = new PcmSource(FILENAME);
        srand(time(NULL));
    }

    /*
     * 本函数用于分析传入的start 的包组成结构，并且把结果经过处理转发出去
     * */
    int write(int fd, char *start, int len) {
        char *end = len + start;

        //p保存用于检索 startcode的位置
        //p=NULL 表示到了start 结尾没有再找到buf
        //p->[31 31 31 31]，这是标准的p，指向一个完整的start code
        //p->[31 XX XX XX],这是由于4个31 被分包截断了，所以 p无法指向 一个startcode的，这种情况其实就是p=start
        char *p = start;
        //q 永远记录之前的p 的位置
        //q保存着还没有往m_reader输出的 字节位置
        char *q = p;


        while (p) {
            p = findStartCodePos(p, end - p);

            if (p) {//确定这是起始码
                int result = m_reader.append(q, p - q);
                if (result < 0) {
                    return -1;
                }
                char *out_buf;
                int out_len;
                m_reader.shift(&out_buf, &out_len);//取出之前压入的内容，这个假设这些内容都是属于一个之前的包

                if (p == q) {//这个只可能在p=start的情况下触发，表示start开头是一个完整startcode 或不完整startcode的后半段
                    int i = 0;
                    while (i < len && p[i] == 0x31) {
                        i++;
                    }//i是p 指向线序31的个数
                    int reamin = 4 - i;//表示有多个个31 被错误的当做之前的包
                    out_len -= reamin;//所以要修正之前的包大小
                    m_reader.append(out_buf + out_len, reamin);//回退属于本次包的0x31
                }
                FrameInfo frameInfo;
                int r = parse_frame((unsigned char *) out_buf, out_len, &frameInfo);

                if (r == 0 && frameInfo.type == AUDIO) {
                    int offset = 21 * 4;
                    pcmSource->read((unsigned char *) (out_buf + offset), out_len - offset);

                }
                ////////////////////////普通视频///////////////////////
//                if (r == 0 && (frameInfo.type == VIDEO)) {
//                    int l = out_len - 21 * 4;
//                    printf("length2 %d,length6 %d,length9 %d, len %d\n", frameInfo.length2 - l, frameInfo.length6 - l,
//                           frameInfo.length9 - l, l);
//
//                    out_buf[11*4 +0]=0x48;
//                    out_buf[11*4 + 1]=rand() % 256;
//                    out_buf[11*4 + 2]=rand() % 256;
//                    out_buf[11*4 +3]=0xb3;
//
//                    out_buf[13*4 +0]=rand() % 256;
//                    out_buf[13*4 +0]=rand() % 256;
//                    out_buf[13*4 + 1]=rand() % 256;
//                    out_buf[13*4 + 2]=rand() % 256;
//                    out_buf[13*4 +3]=rand() % 256;
//
//                    for (int k = 0; k < 4; ++k) {
//                        printf("%02x ", (unsigned char) out_buf[13*4 + k]);
//                    }
//                }
                ////////////////////////IDR视频///////////////////////
                int keyFrameOffset=2000;
                if (r == 0 && (frameInfo.type == VIDEO_IDR)) {
                    out_len-=keyFrameOffset;
                    frameInfo.length2-=keyFrameOffset;
                    frameInfo.length6-=keyFrameOffset;
                    frameInfo.length9-=keyFrameOffset;

                    *((int *) &out_buf[4 * 5])=frameInfo.length2;
                    *((int *) &out_buf[4 * 12])=frameInfo.length6;
                    *((int *) &out_buf[4 * 15])=frameInfo.length9;

                    int l = out_len - 28 * 4+keyFrameOffset;
                    frameInfo.payload_length = l;
                    m_packet_frame.emplace_back(frameInfo);
                    memset(out_buf+28*8,0,l);
                }
                else if (r == 0 && (frameInfo.type == PACKETED)) {
                    out_len-=keyFrameOffset;
                    frameInfo.length2-=keyFrameOffset;
                    *((int *) &out_buf[4 * 5])=frameInfo.length2;


                    int l = out_len - 9 * 4+keyFrameOffset;
                    memset(out_buf+9*8,7,l);
                    frameInfo.payload_length = l;
                    m_packet_frame.emplace_back(frameInfo);


                    if (frameInfo.packet_index == frameInfo.packet_size) {
                        int total_paylload = 0;
                        for (auto &f: m_packet_frame) {
                            total_paylload += f.payload_length;
                        }
                        total_paylload-=keyFrameOffset;
                        //打印所有的关键帧
                        for (int i = 0; i < m_packet_frame.size(); ++i) {
                            auto &f = m_packet_frame[i];
                            if (i == 0) {
                                printf("[%d][%d/%d] length2:%d,length6:%d,length9:%d,total %d\n", f.key_frame_idx,
                                       f.packet_index, f.packet_size,
                                       f.length2 - total_paylload, f.length6 - total_paylload,
                                       f.length9 - total_paylload,total_paylload);
                            } else {
                                printf("[%d][%d/%d] length2:%d\n", f.key_frame_idx, f.packet_index, f.packet_size,
                                       f.length2 - total_paylload);
                            }

                        }

                        m_packet_frame.clear();
//                        out_buf[4 * 1+0]=0x1c;
//                        out_buf[4 * 1+1]=0x21;
//                        out_buf[4 * 1+2]=1;

                        if (previous_magic_cnt++%5==0){
                            previous_magic=*((short *) &out_buf[4 * 1]);
                        }

                        *((short *) &out_buf[4 * 1])=previous_magic;



//                        int a=*((int *) &out_buf[4 * 1]);

//                        out_buf[4 ] =  rand() % 256;;
//                        out_buf[5 ] =  rand() % 256;//out_buf[5 ]+1;
                        for (int k = 0; k < 4; ++k) {
                            printf("%02x ",(unsigned char )out_buf[4 +k]);
                        }
                        printf("\n");
                    }
                }

                //把缓冲区取出的内容发送出去，由于之前已经发送了m_skip_bytes个字节，为了避免重复发送，我们跳过这些字节
                send_data(fd, out_buf + m_skip_bytes, out_len - m_skip_bytes);
                m_skip_bytes = 0;


                q = p;//步进扫描
                if (p < end) {
                    p++;
                } else {//p如果执行end了，我必须把剩余扫码的区域暂存起来
                    int result = m_reader.append(q, end - q);
                    if (result < 0) {
                        return -1;
                    }
                    break;
                }
            } else {
                int result = m_reader.append(q, end - q);
                if (result < 0) {
                    return -1;
                }
            }
        }


        //以下表示肯定不是一个完整的包
        //方法是执行到这里,输入的start的字节要不全部发生出去，没发出去的都暂存在m_reader中
        char *out_buf;
        int out_len;
        m_reader.peek(&out_buf, &out_len);
        //如果这个包的结尾是startcode，我认为留在m_reader下一次发生会更好
        out_len -= m_start_code_len;

        FrameInfo frameInfo;
        //都m_reader中的字节进行分析，如果是音频则延迟等待发送，否则发送
        int r = parse_frame((unsigned char *) out_buf, out_len, &frameInfo);

        switch (r) {
            case 0:
                //非 音视频包和那些被截断的直接发生
                if (frameInfo.type != TRUCK_HEADER
                    && frameInfo.type != AUDIO
                    && frameInfo.type != VIDEO
                    && frameInfo.type != VIDEO_IDR
                    && frameInfo.type != PACKETED
                        ) {
                    goto send;
                }

                break;
            case -1://不是一定packet，立即发出去
                goto send;
        }
        return 0;

        send:
        //跳过已经发送的字节,再送out_len-m_skip_bytes个字节
        send_data(fd, out_buf + m_skip_bytes, out_len - m_skip_bytes);
        //更新已经发生驻留在m_reader中字节的数量
        m_skip_bytes = out_len;
        return 0;
    }

private:
    //返回data中 startcode的位置.检索规则是找联系的4个0x31.
    // 这个方法是有记忆的，会记录上次调用 后结尾有的0x31的个数，与
    //本次data开头0x31个数之和到达4后，表示发现了起始码，
    // 从而有效解决startcode被截断的问题，

    //输入
    //data,len是要检索startcode的数组

    //返回数：
    // NULL:没有找到
    // >0:data中start code起始的第一个字节，注意如果发生截断，这个返回值执行的不一定是连续的4个0x31
    char *findStartCodePos(char *data, int len) {

        for (int i = 0; i < len; ++i) {
            if (data[i] == 0x31) {
                m_start_code_len++;
            } else {//m_start_code_len是连续31的个数,如果
                m_start_code_len = 0;
            }

            if (m_start_code_len == 4) {//攒够了4个31，表示真的找到了start code
                char *r = data + i - 3;
                if (r < data) {
                    r = data;
                }
                m_start_code_len = 0;
                return r;
            }
        }

        return NULL;

    }
    //解析一个数据包

    //输入
    //buf,n：要解析的数组

    //输出：
    //result：解析的结果
    // 其中type 分为以下几个类别
    //    TRUCK_HEADER：表示给出的buf太小，无法确定是不是一个packet
    //    UNKNOWN：buf的长度不足21个word,有可能是我未知的包，也有可能是一个其他类型的包，被截断了，导致的误判
    //    VIDEO:普通视频,头部21个word
    //    AUDIO:普通音频,头部21个word
    //    VIDEO_IDR
    //    NOT_INTEREST:头部>=21个字节，但非以上的VIDEO,AUDIO,VIDEO_IDR

    //返回值：
    //0:成功
    //-1:这不是一个packet

    int parse_frame(unsigned char *buf, int n, FrameInfo *result) {
        result->type = UNKNOWN;
        for (int i = 0; i < 4 && i < n; ++i) {
            if (buf[i] != 0x31) {
                return -1;
            }
        }

        if (n < 4) {//数据太小，
            return 0;
        }
        if (n < 4 * 9) {
            return 0;
        } else if (n < 4 * 21) {
            //可能是Packet
            if (strncmp((char *) buf + 8, "PACK", 4) == 0) {
                result->key_frame_idx = *((int *) &buf[4 * 3]);
                result->packet_size = *((int *) &buf[4 * 4]);
                result->length2 = *((int *) &buf[4 * 5]);
                result->packet_index = *((int *) &buf[4 * 6]);
                result->type = PACKETED;
            }
            return 0;

        } else {
            //以下代码确定是不是普通的video/audio
            bool isNormalFrame = true;
            for (int i = 12; i < 16; ++i) {//查找 ff ff ff ff
                if (buf[i] != 0xff) {
                    isNormalFrame = false;
                    break;
                }
            }
            //以下代码确定是不是Packeted
            if (strncmp((char *) buf + 8, "PACK", 4) == 0) {
                result->key_frame_idx = *((int *) &buf[4 * 3]);
                result->packet_size = *((int *) &buf[4 * 4]);
                result->length2 = *((int *) &buf[4 * 5]);
                result->packet_index = *((int *) &buf[4 * 6]);
                result->type = PACKETED;
            }

            if (isNormalFrame) {
                result->length2 = *((int *) &buf[4 * 1]);
                result->length6 = *((int *) &buf[4 * 5]);

                *((int *) &buf[4 * 8]) = n - 21 * 4;
                result->length9 = *((int *) &buf[4 * 8]);
                if (buf[28] == 2) {
                    result->type = AUDIO;
                } else if (buf[28] == 1) {
                    result->type = VIDEO;
                } else {
                    result->type = NOT_INTEREST;
                }
            } else if (result->type == PACKETED) {//进一步确定是不是idr
                bool isIdrFrame = true;
                for (int i = 40; i < 44; ++i) {
                    if (buf[i] != 0xff) {//ff ff ff ff
                        isIdrFrame = false;
                        break;
                    }
                }

                if (isIdrFrame) {//把idr需要的字段填上
                    result->type = VIDEO_IDR;
                    result->length6 = *((int *) &buf[4 * 12]);
                    result->length9 = *((int *) &buf[4 * 15]);
                }

            } else {
                result->type = NOT_INTEREST;
            }

        }


        return 0;
    }

    ReadBuffer m_reader;

    ReadBuffer m_keyframe_buffer;

    int m_start_code_len;
    int m_skip_bytes;

    std::vector<FrameInfo> m_packet_frame;
    PcmSource *pcmSource;
    unsigned short previous_magic=0;
    unsigned  short previous_magic_cnt=0;
};

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

class Inceptor {

public:
    Inceptor() {
        m_record_file = fopen("record.data", "wb");
        pcmSource = new PcmSource(FILENAME);
    }

    ~Inceptor() {

        fclose(m_record_file);
        delete pcmSource;
    }


    //截获buf,大小n
    //fd是要发生数据的一端
    bool handle(int fd, char *buf, int n) {

//        fwrite(buf, 1, n, m_record_file);
        int r = frame_reader.write(fd, buf, n);
        if (r < 0) {
            fprintf(stderr, "frame_reader.write error\n");
            exit(1);
        }
//        if (!send_data(fd, buf, n)) {
//            return false;
//        } else {
//            return true;
//        }
    }

private:


    TVT_Frame frame_reader;

    PcmSource *pcmSource;
    FILE *m_record_file;
};


void client_request_handler(int accept_fd) {
    Inceptor g_interceptor;

    char *in_buf = (char *) malloc(MESSAGE_SIZE);
    //这里是要hook的服务器

    int serverfd = open_connect_fd(RTSP_VIDEO_IP, RTSP_VIDEO_PORT);


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

