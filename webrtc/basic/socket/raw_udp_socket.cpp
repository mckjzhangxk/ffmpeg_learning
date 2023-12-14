#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

//生成ip的校验和,ip只会校验ip header,因为ttl每次经过路由都会变化
//所以这个数值也是变化的
uint16_t checksum_generic(uint16_t *addr, uint32_t count) {
    register unsigned long sum = 0;
    //从后往前相加，每个word为一个元素
    for (sum = 0; count > 1; count -= 2)
        sum += *addr++;
    if (count == 1)
        sum += (char) *addr; //如果是奇数，扩展addr[0] --> 0x00 addr[0]

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

//udp校验和生成的数据是以下数据的checksum
// 1.udp header
// 2.udp payload
//以下是伪ip数据包
// 3.srcip
// 4.desip
// 5.protocal
// 6.udp数据包长度:注意这里是大端模式
uint16_t checksum_tcpudp(struct ip *iph, void *buff, int buff_len, uint16_t data_len) {
    const uint16_t *buf = (const uint16_t *) buff;
    uint32_t ip_src = iph->ip_src.s_addr;
    uint32_t ip_dst = iph->ip_dst.s_addr;
    uint32_t sum = 0;

    while (buff_len > 1) {
        sum += *buf;
        buf++;
        buff_len -= 2;
    }

    if (buff_len == 1)
        sum += *((uint8_t *) buf);

    sum += (ip_src >> 16) & 0xFFFF;
    sum += ip_src & 0xFFFF;
    sum += (ip_dst >> 16) & 0xFFFF;
    sum += ip_dst & 0xFFFF;
    sum += htons(iph->ip_p); //这里是大端  0x11 0x00
    sum += data_len;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ((uint16_t) (~sum));
}

bool test_checksum_generic(uint16_t *addr, uint32_t count) {
    register unsigned long sum = 0;

    for (sum = 0; count > 1; count -= 2)
        sum += *addr++;
    if (count == 1)
        sum += (char) *addr;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return sum == 0xffff;
}

int open_connect_fd() {
    //////////////////////////////原始套接字创建///////////////////////////
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        cerr << "socket create error" << endl;
        exit(1);
    }

    //////////////////////////////套接字属性设置///////////////////////////
    int on = 1;
    // IP_HDRINCL.在不设置这个选项的情况下，IP协议自动填充IP数据包的首部。
    if (setsockopt(sockFd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(int)) < 0) {
        cerr << "fail to custom packet" << endl;
        exit(1);
    }
    //非阻塞设置
    fcntl(sockFd, F_SETFD, fcntl(sockFd, F_GETFD) | O_NONBLOCK);

    return sockFd;
}

void create_tcp_packet(
        struct sockaddr_in &src_addr, //?????ip,port
        struct sockaddr_in &dst_addr, //??????ip,port
        char *&out_packet,            //???
        int &out_packet_len) {        //?????

    static short id = 0;
    out_packet_len = sizeof(struct ip) + sizeof(struct tcphdr) + 20;
    out_packet = new char[out_packet_len];
    struct ip *ipheader = (struct ip *) out_packet;
    struct tcphdr *tcphdr = (struct tcphdr *) (ipheader + 1);
    int8_t *opts = (int8_t *) (tcphdr + 1);

    /////////ip packet////////////////////
    //4 bytes
    ipheader->ip_v = 4;
    ipheader->ip_hl = 5;//??4?bytes
    ipheader->ip_tos = 0;//??
    ipheader->ip_len = htons(out_packet_len);

    //4 bytes
    ipheader->ip_id = htons(id++);
    ipheader->ip_off = htons(1 << 14); //??don't fragment????????????????

    //4 bytes
    ipheader->ip_ttl = 64;
    ipheader->ip_p = IPPROTO_TCP;
    ipheader->ip_sum = 0;//????checksum


    ipheader->ip_src = src_addr.sin_addr;
    ipheader->ip_dst = dst_addr.sin_addr;

    ///tcp header
#ifdef __linux__

    tcphdr->source=src_addr.sin_port;
    tcphdr->dest=dst_addr.sin_port;
    tcphdr->seq= htons(0);
    tcphdr->ack_seq= htons(0);
    tcphdr->doff = 10;//data offset,???4bytes?????4*10=40=20(fixed)+20(option)
    tcphdr->syn=1;
    tcphdr->window= htons(29200);//65536
#elif defined(__APPLE__) && defined(__MACH__)
    tcphdr->th_sport = src_addr.sin_port;
    tcphdr->th_dport = dst_addr.sin_port;
    tcphdr->th_seq = htons(0);
    tcphdr->th_ack = htons(0);
    tcphdr->th_off = 10;//data offset,???4bytes?????4*10=40=20(fixed)+20(option)
    tcphdr->th_flags = TH_SYN;
    tcphdr->th_win = 0xffff;//65536
#endif


    //20????tcp option
    //4 bytes
    // TCP MSS
    *opts++ = 2;    // Kind PROTO_TCP_OPT_MSS
    *opts++ = 4;                    // Length
    *((uint16_t *) opts) = htons(1400 + (100 & 0x0f));
    opts += sizeof(uint16_t);

    //2 bytes TCP SACK permitted
    *opts++ = 4;//PROTO_TCP_OPT_SACK;
    *opts++ = 2;

    //2+4+4=10 bytes
    *opts++ = 8;//PROTO_TCP_OPT_TSVAL;
    *opts++ = 10;
    *((uint32_t *) opts) = htons(99);
    opts += sizeof(uint32_t);
    *((uint32_t *) opts) = 0;
    opts += sizeof(uint32_t);

    // TCP nop, 1 byte
    *opts++ = 1;

    // TCP window scale
    //3 bytes
    *opts++ = 3;//PROTO_TCP_OPT_WSS;
    *opts++ = 3;
    *opts++ = 7; // 2^6 = 64, window size scale = 64

    ipheader->ip_sum = 0;
    ipheader->ip_sum = checksum_generic((uint16_t *) out_packet, sizeof(struct ip));

#ifdef __linux__
    tcphdr->check=0;
    tcphdr->check=checksum_tcpudp(ipheader,tcphdr, sizeof(struct tcphdr)+20, htons(sizeof(struct tcphdr)+20));
#elif defined(__APPLE__) && defined(__MACH__)
    tcphdr->th_sum = 0;
    tcphdr->th_sum = checksum_tcpudp(ipheader, tcphdr, sizeof(struct tcphdr), htons(sizeof(struct tcphdr)));
#endif

}

void create_udp_packet(
        struct sockaddr_in &src_addr, //构造包的源ip,port
        struct sockaddr_in &dst_addr, //构造包的目的ip,port
        char *data,                   //发送的数据
        int datasize,                 //发送的数据长度
        char *&out_packet,            //输出包
        int &out_packet_len) {        //输出包大小

    static short id = 0;

    out_packet_len = sizeof(struct ip) + sizeof(struct udphdr) + datasize;
    out_packet = new char[out_packet_len];
    struct ip *ipheader = (struct ip *) out_packet;
    struct udphdr *udphdr = (struct udphdr *) (ipheader + 1);


    //copy data
    memcpy(udphdr + 1, data, datasize);

    /////////ip packet////////////////////

    //4 bytes
    ipheader->ip_v = 4;
    ipheader->ip_hl = 5;//但是4个bytes
    ipheader->ip_tos = 0;//??
    ipheader->ip_len = htons(out_packet_len);

    //4 bytes
    ipheader->ip_id = htons(id++);
    ipheader->ip_off = htons(1 << 14); //设置don't fragment，之前没有转大端，导致发送不出去

    //4 bytes
    ipheader->ip_ttl = 64;
    ipheader->ip_p = IPPROTO_UDP;
    ipheader->ip_sum = 0;//不要检查checksum


    ipheader->ip_src = src_addr.sin_addr;
    ipheader->ip_dst = dst_addr.sin_addr;


    /////////udp packet////////////////////
#ifdef __linux__

    udphdr->source= src_addr.sin_port;
    udphdr->dest=dst_addr.sin_port;
    udphdr->len=htons(sizeof(struct udphdr )+datasize);
    udphdr->check=0;
#elif defined(__APPLE__) && defined(__MACH__)
    udphdr->uh_sport = src_addr.sin_port;//已经是大端模式
    udphdr->uh_dport = dst_addr.sin_port;
    udphdr->uh_ulen = htons(sizeof(struct udphdr) + datasize);
    udphdr->uh_sum = 0;  //之前设置有问题导致发包收不到
#endif
    //校验和生成
    ipheader->ip_sum = checksum_generic((uint16_t *) out_packet, sizeof(struct ip));
#ifdef __linux__
    udphdr->check=0;
    //之前忽视了udphdr->len已经是大端模式，错误把udphdr->len当做了udphdr数据包大小
    udphdr->check=checksum_tcpudp(ipheader,udphdr,ntohs(udphdr->len),udphdr->len);
    cout<<udphdr->check<<endl;
#elif defined(__APPLE__) && defined(__MACH__)
    udphdr->uh_sum = checksum_tcpudp(ipheader, udphdr, ntohs(udphdr->uh_ulen), udphdr->uh_ulen);
#endif

}

/**
 * 本程序无法在mac上运行
 *
 *
 * */
int main() {
    int sockfd = open_connect_fd();
    struct sockaddr_in src_addr, dst_addr;

    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(1234);
    inet_aton("192.168.126.99", &src_addr.sin_addr);

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(8888);
    inet_aton("192.168.126.239", &dst_addr.sin_addr);


    char data[] = "hello world";
    char *packet;
    int packsize;
    create_tcp_packet(src_addr, dst_addr, packet, packsize);

    create_udp_packet(src_addr, dst_addr, data, strlen(data), packet, packsize);
    int n = sendto(sockfd, packet, packsize, MSG_NOSIGNAL, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
    cout << n << endl;
    close(sockfd);
}