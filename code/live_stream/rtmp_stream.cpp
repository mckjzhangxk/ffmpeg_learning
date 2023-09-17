#include <librtmp/rtmp.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

FILE * open_flv(const char * filename){
    FILE *fp=fopen(filename,"rb");

    /**
     * FLV Header占9字节
     *
     * 第1-3字节 F L V 三个字符
     * 第4字节   0x01  保留字
     * 第5字节  头5位00000
     * 5[6]  是否包含音频
     * 5[7]  0 保留
     * 5[8] 是否包含视频频
     * 第6-9字节 0x00 00 00 09，表示FLV Header长度，一定是9
     * */
    fseek(fp,9,SEEK_SET); //跳过 FLV Header
    return fp;

}

RTMP * connect_rtmp(char * addr){
    //1.创建对象
    RTMP *rtmp=RTMP_Alloc();
    RTMP_Init(rtmp);

    //2.参数设置
    rtmp->Link.timeout=10;
    RTMP_SetupURL(rtmp,addr);
    RTMP_EnableWrite(rtmp);  //执行本函数表示推流，否则表示拉流

    //3.建立连接
    if(!RTMP_Connect(rtmp,NULL)){
        fprintf(stderr,"建立连接失败");
        goto ERROR;
    }

    //4.创建流
    if(!RTMP_ConnectStream(rtmp,0)){
        fprintf(stderr,"无法创建流");
        goto ERROR;
    }



    return rtmp;
    ERROR:
    if (rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
}


//注意 pFile先读到的是 高位，后读到的是低位
void read_n_byte(FILE *pFile, unsigned int & buf,int n){
    char* bb=(char *)&buf;

//    for (int i = n-1; i >=0 ; i--) {
//        fread(bb+i,1,1,pFile);
//    }

    //高效的实现
    fread(bb,1,n,pFile);
    for (int i = 0; i <n/2 ; ++i) {
        char t=bb[i];
        bb[i]=bb[n-1-i];
        bb[n-1-i]=t;
    }
}

//把flv的 tag_header ->rtmp packet header
//把flv的 tag_data ->rtmp packet body
int read_data(FILE *pFile, RTMPPacket *pPacket) {
    static int packIndex=0;
    //跳过pretag_size
    if (!feof(pFile)){
        fseek(pFile,4,SEEK_CUR);
    } else{
        return -2;
    }

    //flv文件格式，tag header
    unsigned int tt=0;  //1字节  包类型。音频(0x08)，视频(0x09)，脚本(0x12)
    unsigned int tag_body_size=0; //3字节  //tag body的长度
    unsigned int ts=0;//4字节，单位毫秒,[扩展1字节 时间戳3字节]
    unsigned int ts_extend=0;
    unsigned int sid=0;   //3字节

    //flv的tag header 转换成rtmp 的packet header的过程
    read_n_byte(pFile,tt,1);
    read_n_byte(pFile,tag_body_size,3);
    read_n_byte(pFile,ts,3);
    read_n_byte(pFile,ts_extend,1);
    read_n_byte(pFile,sid,3);

    ts=ts|(ts_extend<<24);

//    printf("%d:type %d,tag_size %d,ts %d,sid %d\n",packIndex++,tt,tag_body_size,ts,sid);
    //rmtp basic header
    pPacket->m_headerType=RTMP_PACKET_SIZE_LARGE; //fmt字段
    //rtmp message header
    pPacket->m_nTimeStamp=ts;//TimeStamp
    pPacket->m_nBodySize=tag_body_size;//Msg Length
    pPacket->m_packetType=tt;//Type ID
     //stream Id??


    //flv的tag data 转换成rtmp 的packet body的过程
    int n=fread(pPacket->m_body,1,tag_body_size,pFile);
    if (n!=tag_body_size){
        fprintf(stderr,"读取flv文件失败");
        return -1;
    }
    return 0;
}

void send_data(FILE * fp,RTMP * rtmp){
    RTMPPacket* packet=new RTMPPacket();

    //分配Packet内部存数据的
    RTMPPacket_Alloc(packet,64*1024);
    RTMPPacket_Reset(packet);
    packet->m_hasAbsTimestamp=0;
    packet->m_nChannel=0x4;

    unsigned int pts=0;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    // 计算从某个点开始的时间戳（以毫秒为单位）
    long long start_timestamp_ms = start_time.tv_sec * 1000LL + start_time.tv_nsec / 1000000LL;
    struct timespec current_time;

    while (1){

        int ret=read_data(fp,packet);

        if (ret<0){
            break;
        }
        if (!RTMP_IsConnected(rtmp)){
            printf("RTMP连接断开\n");
            break;
        }


        clock_gettime(CLOCK_MONOTONIC, &current_time);
        // 以系统时间戳system_pts为基准，如果packet的时间戳 晚于system_pts，等待 一定时间后再发生
        long long current_timestamp_ms = current_time.tv_sec * 1000LL + current_time.tv_nsec / 1000000LL;
        long long system_pts=current_timestamp_ms-start_timestamp_ms;
        if (packet->m_nTimeStamp>system_pts){
            usleep((packet->m_nTimeStamp-system_pts)*1000);
        }

//        printf("current system %ld, current video timestamp %ld\n",system_pts,packet->m_nTimeStamp);
        RTMP_SendPacket(rtmp,packet,0);


    }

    RTMPPacket_Free(packet);
}


int main(int argc,char * argv[]){

    FILE *fp= open_flv(argv[1]);
    RTMP * rtmp= connect_rtmp(argv[2]);
    send_data(fp,rtmp);

    fclose(fp);
}