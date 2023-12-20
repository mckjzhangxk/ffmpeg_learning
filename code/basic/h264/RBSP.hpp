#ifndef EYERH264DEOCDER_RBSP_HPP
#define EYERH264DEOCDER_RBSP_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//RBSP就是NALU header+VCL+padding
class RBSP {
public:
    RBSP(){}
    ~RBSP(){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
    }

    RBSP(const RBSP & rbsp){
        *this = rbsp;
    }
    RBSP & operator = (const RBSP & rbsp){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
        len = rbsp.len;
        buf = (uint8_t *)malloc(len);
        memset(buf,0,len);

        memcpy(buf, rbsp.buf, len);
        return *this;
    }

public:
    uint8_t * buf = nullptr;
    int len = 0;
};

#endif //EYERH264DEOCDER_RBSP_HPP