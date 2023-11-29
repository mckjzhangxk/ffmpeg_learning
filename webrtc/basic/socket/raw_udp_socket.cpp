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
#include <netinet/udp.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

//生成ip的校验和,ip只会校验ip header,因为ttl每次经过路由都会变化
//所以这个数值也是变化的
uint16_t checksum_generic(uint16_t *addr, uint32_t count)
{
    register unsigned long sum = 0;

    for (sum = 0; count > 1; count -= 2)
        sum += *addr++;
    if (count == 1)
        sum += (char)*addr;

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
// 6.udp数据包长度
uint16_t checksum_tcpudp(struct ip *iph, void *buff, uint16_t data_len, int len)
{
    const uint16_t *buf = ( const uint16_t *)buff;
    uint32_t ip_src = iph->ip_src.s_addr;
    uint32_t ip_dst = iph->ip_dst.s_addr;
    uint32_t sum = 0;

    while (len > 1)
    {
        sum += *buf;
        buf++;
        len -= 2;
    }

    if (len == 1)
        sum += *((uint8_t *) buf);

    sum += (ip_src >> 16) & 0xFFFF;
    sum += ip_src & 0xFFFF;
    sum += (ip_dst >> 16) & 0xFFFF;
    sum += ip_dst & 0xFFFF;
    sum += htons(iph->ip_p); //这里是大端
    sum += data_len;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ((uint16_t) (~sum));
}

bool test_checksum_generic(uint16_t *addr, uint32_t count)
{
    register unsigned long sum = 0;

    for (sum = 0; count > 1; count -= 2)
        sum += *addr++;
    if (count == 1)
        sum += (char)*addr;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return sum==0xffff;
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


void create_udp_packet(
        struct sockaddr_in &src_addr, //构造包的源ip,port
        struct sockaddr_in &dst_addr, //构造包的目的ip,port
        char *data,                   //发送的数据
        int datasize,                 //发送的数据长度
        char* &out_packet,            //输出包
        int& out_packet_len) {        //输出包大小

    static short id = 0;

    out_packet_len=sizeof(struct ip) + sizeof(struct udphdr) + datasize;
    out_packet = new char[out_packet_len];
    struct ip * ipheader = (struct ip *) out_packet;
    struct udphdr * udphdr = (struct udphdr *) (ipheader + 1);


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
    ipheader->ip_off = htons(1<<14); //设置don't fragment，之前没有转大端，导致发送不出去

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

    ipheader->ip_sum=checksum_generic((uint16_t*)out_packet,sizeof(struct ip));
    udphdr->uh_sum=checksum_tcpudp(ipheader,udphdr,udphdr->uh_ulen,udphdr->uh_ulen);
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
    char* packet;
    int packsize;
    create_udp_packet(src_addr, dst_addr, data, strlen(data),packet,packsize);


    int n = sendto(sockfd, packet,packsize, MSG_NOSIGNAL,(struct sockaddr *) &dst_addr, sizeof(dst_addr));
    cout<<n<<endl;
    close(sockfd);
}