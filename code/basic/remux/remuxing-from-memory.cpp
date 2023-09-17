#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

//本demo演示了如何从内存读取数据流
//重点是定义 read_package与seek函数
//正确的定义这两个函数，才可以保证输入给
//ffmpeg数据流的正确性
//参考：https://github.com/leixiaohua1020/simplest_ffmpeg_mem_handler/blob/master/simplest_ffmpeg_mem_transcoder/simplest_ffmpeg_mem_transcoder.cpp

class FileObject{
public:
    FileObject(const char * filename){
        mFp = fopen (filename, "rb+");
        fseek(mFp, 0, SEEK_END);
        mFileSize = ftell(mFp);
        fseek(mFp, 0, SEEK_SET);
    }

    size_t Read(uint8_t * buf, int buf_size){
        size_t true_size = fread (buf, 1, buf_size, mFp);
        int position=ftell(mFp);

//        av_log(NULL,AV_LOG_ERROR,"文件位置:%d/%d\n",position,mFileSize);
        return true_size;
    }

    //这个seek很重要，因为我发现ffmpeg在调用
    //av_format_find_stream_info后会回退
    int Seek(long offset, int whence){
        if (whence==AVSEEK_SIZE){
            return mFileSize;
        }
        int v= fseek(mFp,offset,whence);
        return v;
    }
private:
    FILE * mFp;
    long  mFileSize;
};
//以下是avio_alloc_context两个回调,告诉AVIOContext如何读数据
int read_buffer (void * opaque, uint8_t * buf, int buf_size) {
    FileObject* client=(FileObject*)opaque;
    return client->Read(buf,buf_size);
}
int64_t seek(void *opaque, int64_t offset, int whence){
    FileObject* client=(FileObject*)opaque;
    return client->Seek(offset,whence);
}
int write_packet(void *opaque, uint8_t *buf, int buf_size){
    int a;
    a=1;
}

#define BUF_SIZE (16*1024)
//参数1 ../file/test000.avi ../file/hls/test000.mp4 avi
//参数2 /Users/zhanggxk/Desktop/yoga/yoga.mov /Users/zhanggxk/Desktop/yoga/yoga.mp4 mov

//本程序运行的重点是
//1)指定对 fmt参数,(ts,avi,mp4)
//2)设置好avio_alloc_context的read,seek函数

//3)对于read函数，并不需要一次返回buf_size大小的数据，一个一个返回也不会影响解码的结果
//4)对于ts文件 avformat_open_input 读取8200字节，avformat_find_stream_info会去读292000个字节
//  对于avi文件,avformat_find_stream_info会去读到文件结尾个字节
int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);
    int ret, i;

    if (argc < 4) {
        printf("usage: remuxing src desc fmt");
        return 1;
    }

    const char * in_filename  = argv[1];
    const char * out_filename = argv[2];
    const char * fmt = argv[3];

    av_register_all();
    FileObject* client=new FileObject(in_filename);


    AVFormatContext *ifmt_ctx =  avformat_alloc_context ();
    //给AVIOContext分配缓冲区，记录读取的数据
//    unsigned char * aviobuffer = (unsigned char *) av_malloc (BUF_SIZE);
    unsigned char * aviobuffer =new unsigned char [BUF_SIZE];
    AVIOContext * avio = avio_alloc_context (aviobuffer, BUF_SIZE,0, client, read_buffer, write_packet, seek);
    ifmt_ctx-> pb = avio;
    ifmt_ctx->flags=AVFMT_FLAG_CUSTOM_IO;
    //注意这里需要设置fmt
//    ifmt_ctx=NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, av_find_input_format(fmt), 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Could not open input file '%s\n",in_filename);
        return -1;
    }
    //



    av_log(NULL,AV_LOG_WARNING,"输入文件的格式名称'%s，全名：%s\n", ifmt_ctx->iformat->name, ifmt_ctx->iformat->long_name);



    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Failed to retrieve input stream information %s\n",in_filename);
        return -1;
    }
    av_dump_format(ifmt_ctx, 0, in_filename, 0);



    AVFormatContext  *ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        av_log(NULL,AV_LOG_ERROR,"Could not create output context:%s\n",out_filename);
        return -1;
    }

    av_log(NULL,AV_LOG_WARNING,"输出文件的格式名称'%s，全名：%s\n", ofmt_ctx->oformat->name, ofmt_ctx->oformat->long_name);


    //输入与输出之间的对应关系


    //流的基本信息
    int stream_index = -1;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {

        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type!=AVMEDIA_TYPE_VIDEO){
            continue;
        }
        if (!avformat_query_codec(ofmt_ctx->oformat,in_codecpar->codec_id,FF_COMPLIANCE_NORMAL)){
            continue;
        }

        stream_index=i;

        AVStream * out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL,AV_LOG_ERROR,"Failed allocating output stream:%s\n",out_filename);
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            av_log(NULL,AV_LOG_ERROR,"Failed to copy codec parameters:%s\n",in_filename);
            return -1;
        }
        out_stream->codecpar->codec_tag = 0;
        break;
    }

    if (stream_index==-1){
        av_log(NULL,AV_LOG_ERROR,"Fail to find compatible video stream:%s\n",in_filename);
        return -1;
    } else{
        av_dump_format(ofmt_ctx, 0, out_filename, 1);
    }


    //当输出格式的 AVFMT_NOFILE标志=1,调用者不需要提供一个AVIOContext,比如m3u8
    //否则 AVFMT_NOFILE标志=0，调用者需要用avio_open 提供一个AVIOContext,比如mp4

    int no_need_provide_io_context =ofmt_ctx->oformat->flags & AVFMT_NOFILE;

    if (!no_need_provide_io_context) {
        // 设置分段相关参数



        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL,AV_LOG_ERROR,"Could not open output file '%s\n",out_filename);
            return -1;
        }
    }


    //设置hls生成的参数
    av_opt_set_int(ofmt_ctx->priv_data,"hls_time", 120, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(ofmt_ctx->priv_data,"hls_list_size", 4, AV_OPT_SEARCH_CHILDREN);
//
//    //https://zhuanlan.zhihu.com/p/506499639
    av_opt_set(ofmt_ctx->priv_data,"hls_segment_filename", "../file/hls/streamname-%05d.ts", AV_OPT_SEARCH_CHILDREN);




    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL,AV_LOG_ERROR,"Error occurred when opening output file header\n");
        return -1;
    }

    //拷贝包的操作
    while (1) {
        //需要时间基
        AVPacket pkt;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0){
            av_log(NULL,AV_LOG_WARNING," Read Package Error:%s\n", av_err2str(ret));
            break;
        }


        AVStream * in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index!=stream_index) {
            av_packet_unref(&pkt);
            continue;
        }
        pkt.stream_index = 0;
        AVStream  *out_stream = ofmt_ctx->streams[pkt.stream_index];

        /* copy packet */

        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
//        log_packet(ofmt_ctx, &pkt, "out");

        ret = av_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);
    end:
    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);




    return 0;
}
