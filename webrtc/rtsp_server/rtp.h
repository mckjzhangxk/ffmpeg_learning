#ifndef _H264TORTP_H
#define _H264TORTP_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define NAL_MAX     4000000
#define H264        96
#define G711        8

typedef struct rtp_header {
    /* little-endian */
    //我之前一直有疑问，为什么version 存在最后，现在明白了，version是追高为，所以存在最后面
    /* byte 0 */
    uint8_t csrc_len:       4;  /* bit: 0~3 */
    uint8_t extension:      1;  /* bit: 4 */
    uint8_t padding:        1;  /* bit: 5*/
    uint8_t version:        2;  /* bit: 6~7 */
    /* byte 1 */
    uint8_t payload_type:   7;  /* bit: 0~6 */
    uint8_t marker:         1;  /* bit: 7 */
    /* bytes 2, 3 */
    uint16_t seq_no;
    /* bytes 4-7 */
    uint32_t timestamp;
    /* bytes 8-11 */
    uint32_t ssrc;
} __attribute__ ((packed)) rtp_header_t; /* 12 bytes */

typedef struct rtp_packet {
    char interleaved[4]; // rtsp/rtp/rtcp使用同一个socket通道，添加一个Interleaved层进行区分
    uint8_t rtp_packet[1500];         // 柔性数组
}__attribute__ ((packed)) rtp_packet_t;

typedef struct nalu_header {
    /* byte 0 */
    uint8_t type:   5;  /* bit: 0~4 */
    uint8_t nri:    2;  /* bit: 5~6 */
    uint8_t f:      1;  /* bit: 7 */
} __attribute__ ((packed)) nalu_header_t; /* 1 bytes */

typedef struct nalu {
    int startcodeprefix_len;
    unsigned len;             /* Length of the NAL unit (Excluding the start code, which does not belong to the NALU) */
    unsigned max_size;        /* Nal Unit Buffer size */
    int forbidden_bit;        /* should be always FALSE */
    int nal_reference_idc;    /* NALU_PRIORITY_xxxx */
    int nal_unit_type;        /* NALU_TYPE_xxxx */
    char *buf;                /* contains the first byte followed by the EBSP */
    unsigned short lost_packets;  /* true, if packet loss is detected */
} nalu_t;


typedef struct fu_indicator {
    /* byte 0 */
    uint8_t type:   5;
    uint8_t nri:    2;
    uint8_t f:      1;
} __attribute__ ((packed)) fu_indicator_t; /* 1 bytes */


typedef struct fu_indicator265 {
    /* byte 0 */
    uint8_t nuh_layer_id:   1;
    uint8_t type:   6;
    uint8_t f:      1;
} __attribute__ ((packed)) fu_indicator265_t;
typedef struct fu_header265 {
    /* byte 0 */
    uint8_t type:   6;
    uint8_t e:      1;
    uint8_t s:      1;
} __attribute__ ((packed)) fu_header265_t; /* 1 bytes */

typedef struct fu_header {
    /* byte 0 */
    uint8_t type:   5;
    uint8_t r:      1;
    uint8_t e:      1;
    uint8_t s:      1;
} __attribute__ ((packed)) fu_header_t; /* 1 bytes */

typedef struct rtp_package {
    rtp_header_t rtp_package_header;
    uint8_t *rtp_load;
} rtp_t;



//视频源
class RTSPSource{
public:
    RTSPSource(char * f):buffer(NULL){
        filename=f;
        fp = fopen(f, "r");
        fseek(fp, 0, SEEK_END);
        buffer_size = ftell(fp);
        buffer_pos=0;
        buffer = (char*)malloc(buffer_size);
        if (buffer == NULL) {
            perror("Error allocating memory");
            exit(1);
            fclose(fp);
        }
        rewind(fp);

        if (fread(buffer, 1, buffer_size, fp) != buffer_size) {
            perror("Error reading file");
            exit(1);
        }
        fclose(fp);
        fp=NULL;
    }
    ~RTSPSource(){
        if (fp!=NULL)
        fclose(fp);
        if (buffer!=NULL){
            free(buffer);
        }
    }
    int fread1(char * tmpbuf2, size_t size,size_t nitms){

        size_t m=size*nitms;
        if (buffer_pos+m>buffer_size){
            return -1;
        }
        memcpy(tmpbuf2,&buffer[buffer_pos+m],m);
        buffer_pos+=m;
        return nitms;
    }
    int copy_nal_from_file( uint8_t *buf, int *len)
    {
        char tmpbuf[4];     /* i have forgotten what this var mean */
        char tmpbuf2[1];    /* i have forgotten what this var mean */
        int flag = 0;       /* i have forgotten what this var mean */
        int ret;


        *len = 0;

        do {
            ret = fread1(tmpbuf2, 1, 1);
            if (!ret) {
                return -1;
            }
            if (!flag && tmpbuf2[0] != 0x0) {
                buf[*len] = tmpbuf2[0];
                (*len)++;
                // debug_print("len is %d", *len);
            } else if (!flag && tmpbuf2[0] == 0x0) {
                flag = 1;
                tmpbuf[0] = tmpbuf2[0];
            } else if (flag) {
                switch (flag) {
                    case 1:
                        if (tmpbuf2[0] == 0x0) {
                            flag++;
                            tmpbuf[1] = tmpbuf2[0];
                        } else {
                            flag = 0;
                            buf[*len] = tmpbuf[0];
                            (*len)++;
                            buf[*len] = tmpbuf2[0];
                            (*len)++;
                        }
                        break;
                    case 2:
                        if (tmpbuf2[0] == 0x0) {
                            flag++;
                            tmpbuf[2] = tmpbuf2[0];
                        } else if (tmpbuf2[0] == 0x1) {
                            flag = 0;
                            return *len;
                        } else {
                            flag = 0;
                            buf[*len] = tmpbuf[0];
                            (*len)++;
                            buf[*len] = tmpbuf[1];
                            (*len)++;
                            buf[*len] = tmpbuf2[0];
                            (*len)++;
                        }
                        break;
                    case 3:
                        if (tmpbuf2[0] == 0x1) {
                            flag = 0;
                            return *len;
                        } else {
                            flag = 0;
                            break;
                        }
                }
            }

        } while (1);

        return *len;
    } /* static int copy_nal_from_file(FILE *fp, char *buf, int *len) */

    void Reset(){
//        fclose(fp);
//        fp = fopen(filename.data(), "r");
    }
    FILE *fp;
    std::string filename;
    char * buffer;
    long buffer_size;
    long buffer_pos;
};
#endif
