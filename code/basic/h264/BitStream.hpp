#ifndef EYERH264DEOCDER_BITSTREAM_HPP
#define EYERH264DEOCDER_BITSTREAM_HPP

#include <stdio.h>
#include <stdint.h>

class BitStream {
public:
    /**
      * 构造函数可以传入一个 buffer，这里可以直接把 nalu 的 buffer 传入
      */
    BitStream(uint8_t * _buf, int _size){
        p = _buf;
        size = _size;
        start = _buf;
        bits_left = 8;
    }
    ~BitStream(){}

    //读取1位输出
    int ReadU1(){
        int r = 0;
        bits_left--;
        r = ((*p) >> bits_left) & 0x01;
        if (bits_left == 0) {
            p++;
            bits_left = 8;
        }
        return r;
    }
    //去读n位输出
    int ReadU(int n){
        int r = 0;
        int i;
        for (i = 0; i < n; i++) {
            r |= ( ReadU1() << ( n - i - 1 ) );
        }
        return r;
    }
    //读取无符号的哥伦布编码 ue(v),-1表示
    int ReadUE(){
        int r = 0;
        int zero_cnt = 0;
        while((ReadU1() == 0) && (zero_cnt < 32)){
            zero_cnt++;
        }
        //退出说明遇到了1
        if(zero_cnt>=32){
            return -1;
        }
        r = ReadU(zero_cnt);
        r += (1 << zero_cnt) - 1;//
        return r;
    }
    //读取有符号的哥伦布编码  se(v)
    int ReadSE(){
        int r = ReadUE();
        if (r & 0x01) {//符号位
            r = (r+1)/2;
        }
        else {
            r = -(r/2);
        }
        return r;
    }

private:
    // 指向 buffer 开始的位置
    uint8_t * start = nullptr;

    // buffer 的长度（单位 Byte）
    int size = 0;

    // 当前读取到了哪个字节
    uint8_t * p = nullptr;

    // 当前读取到了字节中的还剩下第几位
    int bits_left = 8;
};

#endif //EYERH264DEOCDER_BITSTREAM_HPP