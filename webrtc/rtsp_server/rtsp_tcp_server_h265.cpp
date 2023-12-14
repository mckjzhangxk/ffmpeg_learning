#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <regex>
#include <arpa/inet.h>
#include <fcntl.h>
#ifdef __linux__
#include <wait.h>
#endif
#define MESSAGE_SIZE 65536
#include "rtp.h"
using std::string;
using std::vector;

#define RTP_PAYLOAD_MAX_SIZE        1400
#define SEND_BUF_SIZE               1500
#define NAL_BUF_SIZE                1500 * 250
#define SSRC_NUM                    0xffef24e7
#define  RTP_HEADER_SIZE 12

uint8_t nal_buf[NAL_BUF_SIZE];



int open_listen_fd(int port){

    //////////////////////////////创建套接字///////////////////////////
    //1.创建套接字
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd == -1 ){
        perror("create socket error");
        exit(1);
    }

    //1-2.设置套接字参数
    //SO_REUSEADDR表示启动的时候，不用等待WAIT_TIME，可以直接启动。
    //问题是为什么需要等待WAIT_TIME？
    int on = 1;
    int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if ( ret == -1 ){
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
    ret = bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if(ret == -1 ) {
        perror("bind error");
        exit(1);
    }
    //////////////////////////////监听套接字///////////////////////////
    int backlog = 10; //并发缓冲区大小，可以理解为并发量
    ret = listen(socket_fd, backlog);
    if ( ret == -1 ){
        perror("listen error");
        exit(1);
    }
    return socket_fd;
}

vector<std::string> splitStringByDelimiter(const string &text, const string &delimiter) {
    vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(delimiter, start)) != string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(text.substr(start));
    return tokens;
}


class Buffer{
public:
    Buffer():cnt(0){

    }

    void Write(const char *buf,int size){
        memcpy(buffer+cnt,buf,size);
        cnt+=size;
    }
    void Write(const char *buf){
        int size= strlen(buf);
        Write(buf,size);
    }

    void Write(std::string& msg){
        Write(msg.data(),msg.size());
    }


    void Send(int fd){
        send(fd, (void*)buffer,cnt, 0);
        cnt=0;
    }

    void WriteHeader(){
        Write("RTSP/1.0 200 OK\r\n");
    }
    void WriteTail(){
        Write("\r\n");
    }
    void WriteSequence(string& seqnum){
        auto s="Cseq: "+seqnum+"\r\n";
        Write(s);
    }
    char buffer[MESSAGE_SIZE];
    int cnt;
};

Buffer* g_buffer;

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



void  init_rtsp_header(rtp_header_t *rtp_hdr,uint16_t& seq_num,uint32_t ts_current,int marker=0){

    /*
     * 1. 设置 rtp 头
     */
    rtp_hdr->csrc_len = 0;
    rtp_hdr->extension = 0;
    rtp_hdr->padding = 0;
    rtp_hdr->version = 2;
    rtp_hdr->payload_type = H264;
    rtp_hdr->marker = marker;
    rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
    rtp_hdr->timestamp = htonl(ts_current);
    rtp_hdr->ssrc = htonl(SSRC_NUM);

}

// 1-start 2-end,0
void init_fragment_header(fu_indicator265_t *fu_ind,fu_header265_t *fu_hdr,uint8_t nalu_hdr, int frag_type=0){
    fu_ind->f = (nalu_hdr & 0x80) >> 7;
    fu_ind->type = 49;
    fu_ind->nuh_layer_id=nalu_hdr&0x01;


    fu_hdr->s = 0;
    fu_hdr->e = 0;
    fu_hdr->type =(nalu_hdr&0x7E)>>1;

    switch (frag_type) {
        case 1:  fu_hdr->s=1; break;
        case 2:   fu_hdr->e=1; break;
        default: break;
    }
}

static int h264nal2rtp_send(int framerate, uint8_t *nalu_buf, int nalu_len,RTSP & rtsp)
{
    static uint32_t ts_current = 0;
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
    static uint16_t seq_num = 0;




    if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {
        /*
         * single nal unit
         */

        memset(SENDBUFFER, 0, SEND_BUF_SIZE);

        //1. 设置 rtp 头
        init_rtsp_header(rtp_hdr,seq_num,ts_current);
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

        int nalu_len_minus_two=nalu_len+2; //去掉nalu头部
        uint8_t *payload_buffer=nalu_buf + 2;

        /* nalu 需要分片发送时分割的个数 */
        int fu_pack_num = nalu_len_minus_two % RTP_PAYLOAD_MAX_SIZE ? (nalu_len_minus_two / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len_minus_two / RTP_PAYLOAD_MAX_SIZE;
        /* 最后一个分片的大小 */
        int last_fu_pack_size = nalu_len_minus_two % RTP_PAYLOAD_MAX_SIZE ? nalu_len_minus_two % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;


        for (int fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
            memset(SENDBUFFER, 0, SEND_BUF_SIZE);
            //1. 设置 rtp 头,12个字节
            init_rtsp_header(rtp_hdr,seq_num,ts_current,fu_pack_num==fu_pack_num-1);

            //2. 设置 rtp 荷载头部,3字节
            fu_indicator265_t * fu_ind = (fu_indicator265_t *)&SENDBUFFER[12];
            SENDBUFFER[13]=nalu_buf[1];
            fu_header265_t * fu_hdr = (fu_header265_t *)&SENDBUFFER[14];

            uint8_t nalu_header=nalu_buf[0];
            int flag_type=fu_seq == 0?1:fu_seq ==fu_pack_num - 1?2:0;
            init_fragment_header(fu_ind,fu_hdr,nalu_header,flag_type);

            int data_size=fu_seq ==fu_pack_num - 1?last_fu_pack_size:RTP_PAYLOAD_MAX_SIZE;
            memcpy(SENDBUFFER + 15, payload_buffer+ RTP_PAYLOAD_MAX_SIZE * fu_seq, data_size );//3. 填充nalu内容
            size_t rtp_size = RTP_HEADER_SIZE + 3 + data_size;  /* rtp头 + nalu头 + nalu内容 */


            packet.interleaved[2] = (rtp_size & 0xff00)>>8;
            packet.interleaved[3] = rtp_size & 0xff;

            if (rtsp.Send((void *)&packet,4+rtp_size)<=0){
                return -1;
            }


        }

    }
    return 0;
}

//////////////////命令处理函数////////////
void handler_options(int fd,string seqnum){
    g_buffer->WriteHeader();
    g_buffer->WriteSequence(seqnum);

    char DateBuf[200];
    time_t tTime = time(NULL);
    memset(DateBuf,0,sizeof(DateBuf));
    strftime(DateBuf, sizeof(DateBuf), "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tTime));
    g_buffer->Write(DateBuf);

    char method[]="Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, PAUSE, RECORD, TEARDOWN\r\n";
    g_buffer->Write(method);

    g_buffer->WriteTail();
    g_buffer->Send(fd);

}
void handler_describe(int fd,string seqnum){
    char * sdp="v=0\n"
               "o=- 0 0 IN IP4 127.0.0.1\n"
               "s=No Name\n"
               "c=IN IP4 127.0.0.1\n"
               "t=0 0\n"
               "a=tool:libavformat 58.20.100\n"
               "m=video 0 RTP/AVP 96\n"
               "b=AS:805\n"
               "a=rtpmap:96 H265/90000\n"
               "a=fmtp:96 sprop-vps=QAEMAf//AWAAAAMAkAAAAwAAAwBdlZgJ; sprop-sps=QgEBAWAAAAMAkAAAAwAAAwBdoAKAgC0WWVmkkyvAQEAAAAMAQAAABkI=; sprop-pps=RAHBcrRiQA==\n"
               "a=control:streamid=0\r\n";
    int sdp_size=strlen(sdp);

    g_buffer->WriteHeader();
    g_buffer->WriteSequence(seqnum);


    char content_type[]="Content-type: application/sdp\r\n";
    g_buffer->Write(content_type);

    //sdp的长度必须包括末尾的\r\n
    string content_len="Content-length: "+ std::to_string(sdp_size)+"\r\n";
    g_buffer->Write(content_len);

    g_buffer->WriteTail();

    g_buffer->Write(sdp);
    g_buffer->Send(fd);
}
void handler_setup(int fd,string seqnum,string& transport){

    std::regex pattern(R"(\d+)");
    std::smatch matches;
    auto s=transport;
    std::vector<int> ports;
    while (std::regex_search(s, matches, pattern)) {
        for (auto match : matches) {
            ports.push_back(std::stoi(match.str()));
            s = matches.suffix().str();
        }
    }




    g_buffer->WriteHeader();
    g_buffer->WriteSequence(seqnum);

    string session="Session: 19122202\r\n";
    g_buffer->Write(session);

    //Transport: RTP/AVP/UDP;unicast;client_port=12464-12465
    string send_transport="Transport: "+transport+";interleaved=0-1\r\n";
    g_buffer->Write(send_transport);

    g_buffer->WriteTail();
    g_buffer->Send(fd);
}
void handler_play(int fd,string seqnum,string session){
    g_buffer->WriteHeader();
    g_buffer->WriteSequence(seqnum);

    session="Session: "+ session+"\r\n";
    g_buffer->Write(session);

    g_buffer->WriteTail();
    g_buffer->Send(fd);
}
//////////////////命令处理函数////////////


void client_handler(int accept_fd){
    char in_buf[MESSAGE_SIZE] = {0,};
    RTSP rtsp(accept_fd);
    while (true){
        memset(in_buf, 0, MESSAGE_SIZE);
        //一直阻塞，直到新数据或者对端关闭？
        int n = recv(accept_fd ,&in_buf, MESSAGE_SIZE, 0);
        if(n == 0){
            printf("EOF\n");
            break;
        }
        string msg=in_buf;

        std::cout<<msg<<std::endl;

        auto vs=splitStringByDelimiter(msg,"\r\n");
        //解析请求头
        std::istringstream iss(vs[0]);
        std::string method, url, version;
        iss >> method >> url >> version;
//        std::cout<<method<<" "<<url<<std::endl;


        //解析其他
        std::map<std::string, std::string> keyValueMap;
        for (int i = 1; i < vs.size()-1; ++i) {
            auto& input= vs[i];
            if (input.empty()){
                continue;
            }
            size_t colonPos =input.find(':');
            if (colonPos != std::string::npos) {
                std::string key = input.substr(0, colonPos);
                std::string value = input.substr(colonPos + 2);

                keyValueMap[key] = value;
            }
        }


        if (method=="OPTIONS"){
            handler_options(accept_fd,keyValueMap["CSeq"]);
        } else if (method=="DESCRIBE"){
            handler_describe(accept_fd,keyValueMap["CSeq"]);
        } else if (method=="SETUP"){
            handler_setup(accept_fd,keyValueMap["CSeq"],keyValueMap["Transport"]);
        } else if(method=="PLAY"){
            handler_play(accept_fd,keyValueMap["CSeq"],keyValueMap["Session"]);

            //start push stream
            RTSPSource source_1("/Users/zhanggxk/Desktop/测试视频/jojo.h265");
//            RTSPSource source_2("/Users/zhanggxk/Desktop/测试视频/my.h265");
            int len;
            int cnt=0;
            int ret=0;


            while (1){
                printf("%d\n",cnt++);
                ret=source_1.copy_nal_from_file(nal_buf, &len);
                if (ret<0){
                    break;
                }
                ret=h264nal2rtp_send(25, nal_buf, len, rtsp);
                if (ret<0){
                    break;
                }
                usleep(1000*25);
            }
        }
    }

    printf("客户端退出\n");
    close(accept_fd);
}




void sigpipe_handler(int sig){

}

int main(int argc,char *argv[]){
    signal(SIGPIPE, sigpipe_handler);
    g_buffer=new Buffer;
    int socket_fd=open_listen_fd(7778);
    while (true){
        struct sockaddr_in  remote_addr;
        socklen_t addr_len = sizeof( struct sockaddr_in );

        int accept_fd = accept( socket_fd, (struct sockaddr *)&remote_addr, &addr_len );
        if (accept_fd<0){
            continue;
        }
        //创建子进程
        client_handler(accept_fd);
    }
}
