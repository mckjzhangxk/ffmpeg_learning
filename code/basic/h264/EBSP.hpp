#ifndef EYERH264DEOCDER_EBSP_HPP
#define EYERH264DEOCDER_EBSP_HPP

#include <stdint.h>

#include "RBSP.hpp"

//EBSP 是在RBSP的基础之上加入了防竞争码， 具体规则是
//当发现一个 00 00 0x 后，追加一个03，编程 00 00 03 0x

//x表示 1，2，3
// 00 00 01  -> 00 00 03 01
// 00 00 02  -> 00 00 03 02
// 00 00 03  -> 00 00 03 03
class EBSP {
public:
    EBSP(){}
    ~EBSP(){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
    }

    EBSP(const EBSP & ebsp){
        *this = ebsp;
    }

    EBSP & operator = (const EBSP & ebsp){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
        len = ebsp.len;
        buf = (uint8_t *)malloc(len);
        memcpy(buf, ebsp.buf, len);
        return *this;
    }
    //解析成为 rbsp的规则是： 当发现 00 00 03 0x的时候，跳过这个03，得到  00 00 0x
    //x 是 1 2 3
    int GetRBSP(RBSP & rbsp){
        rbsp.len = len;
        rbsp.buf = (uint8_t *)malloc(rbsp.len);

        int targetIndex=0;
        for(int i=0; i<len; i++){
            if(buf[i] == 0x03 && (i >=2&buf[i - 2] == 0x00 && buf[i - 1] == 0x00) && ( (i+1<len) &&buf[i + 1]<=3 ) ){
                    // 满足条件，该位置不进行拷贝
                    //条件 00 00 03 0x,x 是 1 2 3
                    rbsp.len--;
            } else{
                rbsp.buf[targetIndex] = buf[i];
                targetIndex++;
            }
        }
        return 0;
    }

public:
    uint8_t * buf = nullptr;
    int len = 0;
};

#endif //EYERH264DEOCDER_EBSP_HPP