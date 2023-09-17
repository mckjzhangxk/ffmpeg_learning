//__STDC_CONSTANT_MACRO已经在允许c++使用C99定义的宏
//因为ffmppeg可能已经使用的这个C99宏，所以一定要定义

//参考以下链接
//https://www.it1352.com/2779907.html
#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
extern "C"{
#endif
#include <stddef.h>
#include "libavutil/log.h"

#ifdef __cplusplus
}
#endif


//#define NULL 0
int main() {
    av_log_set_level(AV_LOG_INFO);
    av_log(NULL,AV_LOG_DEBUG,"Http request error:%s\n","url not found");
    av_log(NULL,AV_LOG_INFO,"Http request error:%s\n","url not found");
    av_log(NULL,AV_LOG_WARNING,"Http request error:%s\n","url not found");
    av_log(NULL,AV_LOG_ERROR,"Http request error:%s\n","url not found");

    return 0;
}
