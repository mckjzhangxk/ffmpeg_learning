
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif

#include "x264.h"

#ifdef __cplusplus
}
#endif
//https://blog.csdn.net/leixiaohua1020/article/details/42078645
//https://blog.csdn.net/mytzs123/article/details/122262837
int main(int argc, char*argv[]){
    FILE* fp_src  = fopen("video.yuv", "rb");
    FILE* fp_dst = fopen("frame.h264", "wb");


    //Encode 50 frame
    //if set 0, encode all frame
    //基本的编码参数,至少是56帧才可以正确输出？？
    int frame_num=56;
    int csp=X264_CSP_I420; //yuv420p
    int width=2940,height=1912;


    x264_picture_t* pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
    x264_picture_t* pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
    x264_param_t* pParam = (x264_param_t*)malloc(sizeof(x264_param_t));


    //x264_param_default()：设置参数集结构体x264_param_t的缺省值。
    x264_param_default(pParam);

    pParam->i_width=width;
    pParam->i_height=height;
    pParam->i_csp=csp;

    x264_param_apply_profile(pParam,x264_profile_names[0]);

    x264_picture_init(pPic_out);
    x264_picture_alloc(pPic_in,csp,width,height);

    x264_t* pHandle=x264_encoder_open(pParam);

    int ysize=width*height;
    if (frame_num==0){

        fseek(fp_src,0,SEEK_END);
        frame_num= ftell(fp_src)/(ysize*3/2);
        fseek(fp_src,0,SEEK_SET);
    }
    for (int i = 0; i < frame_num; ++i) {

        fread(pPic_in->img.plane[0],ysize,1,fp_src);
        fread(pPic_in->img.plane[1],ysize/4,1,fp_src);
        fread(pPic_in->img.plane[2],ysize/4,1,fp_src);

        int iNal   = 0;
        x264_nal_t* pNals = NULL;

        pPic_in->i_pts=i;
        int ret=x264_encoder_encode(pHandle,&pNals,&iNal,pPic_in,pPic_out);
        if (ret<0){
            return -1;
        }
        printf("success encode frame %d\n",i);

        for (int j = 0; j < iNal; ++j) {
            fwrite(pNals[j].p_payload,1,pNals[j].i_payload,fp_dst);
        }
    }

    //flush encoder

    while (1){
        int iNal   = 0;
        x264_nal_t* pNals = NULL;
        int ret=x264_encoder_encode(pHandle,&pNals,&iNal,NULL,pPic_out);
        if (ret==0){
            break;
        }
        printf("success flush frame \n");
        for (int j = 0; j < iNal; ++j) {
            fwrite(pNals[j].p_payload,1,pNals[j].i_payload,fp_dst);
        }
    }

    x264_picture_clean(pPic_in);
    x264_encoder_close(pHandle);

    fclose(fp_src);
    fclose(fp_dst);

    free(pPic_in);
    free(pPic_out);
    free(pParam);
    return 0;
}


