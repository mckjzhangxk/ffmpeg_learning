#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

#ifndef AV_WB32
#   define AV_WB32(p, val) do {                 \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif


// outsize=in_size+sps_pps_size+startcode.size,
//startcode=0001(sps,pps) ,001(normal frame)
//memory layout:
//[sps_pps,startcode,in]

//把sps_pps,in的数据按照顺序追加给 out->data
static int alloc_and_copy(AVPacket *out,
                          const uint8_t *sps_pps, //sps 数据
                          uint32_t sps_pps_size,//sps,pps 数据大小
                          const uint8_t *in, //nalu 一帧的数据
                          uint32_t in_size   //nalu 一帧的数据大小
                          )
{
    uint32_t offset         = out->size;
    uint8_t startcode_size = !sps_pps_size ? 3 : 4;

    //新的一个数据 应该是 spspps+nalu_header+nalu_body
    int err = av_grow_packet(out, sps_pps_size + in_size + startcode_size);
    if (err < 0)
        return err;
    //sps_pps已经自带start code了
    if (sps_pps)
        memcpy(out->data + offset, sps_pps, sps_pps_size);
    //拷贝nala到 sps_pps.size+startcode_size之后
    memcpy(out->data + sps_pps_size +offset+ startcode_size , in, in_size);

    char * startcode_buf=out->data + offset + sps_pps_size;
    if (sps_pps_size) {//关键帧
        startcode_buf[0] =0;
        startcode_buf[1] =0;
        startcode_buf[2] =0;
        startcode_buf[3] =1;

    } else {
        startcode_buf[0] =0;
        startcode_buf[1] =0;
        startcode_buf[2] =1;
    }

    return 0;
}

//根据codec_extradata，生成sps,pps帧,并且追加start_code
//out_extradata就是要追加的
int h264_extradata_to_annexb(const uint8_t *codec_extradata, const int codec_extradata_size, AVPacket *out_extradata, int padding)
{
    uint16_t unit_size;
    uint64_t total_size                 = 0;
    uint8_t *out                        = NULL, sps_done = 0,
            sps_seen                   = 0, pps_seen = 0;
    const uint8_t *extradata            = codec_extradata + 4;
    static const uint8_t start_code_header[4] = { 0, 0, 0, 1 };
    int length_size = (*extradata++ & 0x3) + 1; // retrieve length coded size, 用于指示表示编码数据长度所需字节数

    int START_CODE_SIZE=sizeof(start_code_header);
    int LEN_SIZE=2; //保存pps长度的字段 占2bits

    uint8_t sps_offset =-1, pps_offset = -1;

    /* retrieve sps and pps unit(s) */
    uint8_t unit_nb = *extradata++ & 0x1f; //获得有多少个sps
    if (!unit_nb) {
        //没有sps，直接掉到pps
        goto pps;
    }else {
        sps_offset = 0;
        sps_seen = 1;
    }

    while (unit_nb--) {
        int err;

        unit_size   = AV_RB16(extradata);//2位单元大小,获得sps,pps数据的大小，sps是用2个字节记录数据的大小
        total_size += unit_size + START_CODE_SIZE;//单元大小+4个开始位置，total是sps,pps+start_code的长度
        if (total_size > INT_MAX - padding) {
            av_log(NULL, AV_LOG_ERROR,
                   "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if (extradata + LEN_SIZE + unit_size > codec_extradata + codec_extradata_size) {//unit_size大小的问题
            av_log(NULL, AV_LOG_ERROR, "Packet header is not contained in global extradata, "
                                       "corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if ((err = av_reallocp(&out, total_size + padding)) < 0)
            return err;
        memcpy(out + total_size - unit_size - START_CODE_SIZE, start_code_header, START_CODE_SIZE);
        memcpy(out + total_size - unit_size, extradata + LEN_SIZE, unit_size);
        extradata += LEN_SIZE + unit_size;
        pps:
        if (!unit_nb && !sps_done++) {
            unit_nb = *extradata++; /* number of pps unit(s) */
            if (unit_nb) {
                pps_offset = total_size;
                pps_seen = 1;
            }
        }
    }

    if (out)
        memset(out + total_size, 0, padding);

    if (!sps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: SPS NALU missing or invalid. "
               "The resulting stream may not play.\n");

    if (!pps_seen)
        av_log(NULL, AV_LOG_WARNING,
               "Warning: PPS NALU missing or invalid. "
               "The resulting stream may not play.\n");

    out_extradata->data      = out;
    out_extradata->size      = total_size;

    return length_size;
}
//avcc -> annexb
// 1.对输入包in，加上start code

// 2.对于 idr的in,再加上sps,pps帧（NALU）

// 3.生成的out包，输出到文件dst_fd

void print_hex(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ",(unsigned char)(buf[i]) );
    }
    printf("\n");
}
//把AVC封装 改成 H264封装（annexb）
// AVC封装：没有开始码的H.264视频，它的数据流开始是1、2或者4个字节表示长度数据
// H264封装（annexb）：有开始码的H.264视频
int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *in, FILE *dst_fd)
{

    AVPacket spspps_pkt;



    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size    = 0;

    int ret = 0;

    AVPacket* out = av_packet_alloc();

    const uint8_t * buf      = in->data;
    int buf_size = in->size;
    const uint8_t * buf_end  = in->data + in->size;

    do {
        ret= AVERROR(EINVAL);
        if (buf + 4 > buf_end)
            goto fail;
        // nal_size是一个h264 frame的长度，由四个字节组成
        // nal_size= buf[3] buf[2] buf[1] buf[0]]
        //frame的长度，指的是一个nalu大小，包括nalu_header+nalu_body

        nal_size=buf[0];
        nal_size=(nal_size << 8) | buf[1];
        nal_size=(nal_size << 8) | buf[2];
        nal_size=(nal_size << 8) | buf[3];

        //nalsize(4字节)+nal header(1字节)
        //nal header结构如下
        // forbit :1bits
        // 主要性:  2bit
        // unit_type: 5bits
        buf += 4;
        unit_type = *buf & 0x1f;

        if (nal_size > buf_end - buf || nal_size < 0)
            goto fail;
//        print_hex(in->data,in->size);
//        print_hex(buf-4,4+nal_size);

        //buf 目前指向nalu的header
        //nal_size 也包括header的长度，
        // (buf,nal_size)=完整的h264包!

        //unit_type:
        // 5-IDR: 关键帧
        // 7-SPS
        // 8-PPS
        // 1-其他帧
        /* prepend only to the first type 5 NAL unit of an IDR picture, if no sps/pps are already present */
        //关键帧前面需要追加PPS,PSP
        if (unit_type == 5) {
            //AVStream->codecs->extradata包括sps,pps信息
            h264_extradata_to_annexb( fmt_ctx->streams[in->stream_index]->codec->extradata,
                                      fmt_ctx->streams[in->stream_index]->codec->extradata_size,
                                      &spspps_pkt,
                                      AV_INPUT_BUFFER_PADDING_SIZE);

            if ((ret=alloc_and_copy(out,spspps_pkt.data, spspps_pkt.size,buf, nal_size)) < 0){
                goto fail;
            }

        }else {
            if ((ret=alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0)
                goto fail;
        }


        buf        += nal_size;
        cumul_size += nal_size + 4;
    } while (cumul_size < buf_size);

    /*
    ret = av_packet_copy_props(out, in);
    if (ret < 0)
        goto fail;

    */

    fwrite( out->data, 1, out->size, dst_fd);
    fflush(dst_fd);

    fail:
    av_packet_free(&out);

    return ret;
}