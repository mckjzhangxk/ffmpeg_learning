#ifndef EYERH264DEOCDER_NALU_HPP
#define EYERH264DEOCDER_NALU_HPP

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "EBSP.hpp"

class Nalu {
public:
    Nalu(){}
    Nalu(const Nalu & nalu){
        *this = nalu;
    }
    ~Nalu(){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
    }

    Nalu & operator = (const Nalu & nalu){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }

        startCodeLen = nalu.startCodeLen;
        len = nalu.len;
        buf = (uint8_t *)malloc(len);
        memcpy(buf, nalu.buf, len);

        forbidden_bit   = nalu.forbidden_bit;
        nal_ref_idc     = nalu.nal_ref_idc;
        nal_unit_type   = nalu.nal_unit_type;


        rbsp = nalu.rbsp;

        return *this;
    }

    int SetBuf(uint8_t * _buf, int _len){
        if(buf != nullptr){
            free(buf);
            buf = nullptr;
        }
        len = _len;

        buf = (uint8_t *)malloc(len);
        memcpy(buf, _buf, len);

        return 0;
    }



    int ParseRBSP(){
        EBSP ebsp;
        GetEBSP(ebsp);
        return ebsp.GetRBSP(rbsp);
    }
    int ParseHeader(){
        uint8_t naluHead    = buf[startCodeLen];
        forbidden_bit       = (naluHead >> 7) & 1;
        nal_ref_idc         = (naluHead >> 5) & 3;
        nal_unit_type       = (naluHead >> 0) & 0x1f;
        return 0;
    }

    int GetNaluType(){
        return nal_unit_type;
    }
    int GetSize(){
        return rbsp.len;
    }
    virtual int Parse(){
        return 0;
    }

public:
    uint8_t * buf = nullptr;
    int len = 0;

    //起始码
    int startCodeLen = 0;
    //第一个字节
    int forbidden_bit = 0;
    int nal_ref_idc = 0;
    int nal_unit_type = 0;
    //完整的nalu的字节
    RBSP rbsp;

private:
    int GetEBSP(EBSP & ebsp){
        ebsp.len = len - startCodeLen-1;
        ebsp.buf = (uint8_t *)malloc(ebsp.len);

        memcpy(ebsp.buf, buf + startCodeLen+1, ebsp.len);


        return 0;
    }
};

#endif //EYERH264DEOCDER_NALU_HPP