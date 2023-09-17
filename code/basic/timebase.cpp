#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif



#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#ifdef __cplusplus
}
#endif
int main(){
    AVRational rv={1,30};
    double d=av_q2d(rv);
    printf("%f \n",d);
    av_usleep(1e6);
}