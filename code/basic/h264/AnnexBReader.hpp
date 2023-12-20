#ifndef EYERLIB_EYER_AVC_BLOG_ANNEXBREADER_HPP
#define EYERLIB_EYER_AVC_BLOG_ANNEXBREADER_HPP

#include <stdint.h>
#include <string>

#include "Nalu.hpp"

class AnnexBReader
{
public:
    AnnexBReader(const std::string & _filePath):bufferLen(0),buffer(nullptr){
        f = fopen(_filePath.c_str(), "rb");
    }
    ~AnnexBReader(){
        Close();
    }

    // 用来关闭文件
    int Close(){
        if(f != nullptr){
            fclose(f);
            f = nullptr;
        }

        freeBuffer();
        return 0;
    }

    // 用来读取一个 Nalu
    //0表示成功，1表示是最后一个了
    int ReadNalu(Nalu & nalu){

        while (1){
            uint8_t * buf = buffer;


            int startCodeLen = 0;
            //默认buf应该是指向起始码的
            CheckStartCode(startCodeLen, buf, bufferLen);

            nalu.startCodeLen = startCodeLen;

            //去找下一个起始码
            int endPos = -1;
            for (int i = nalu.startCodeLen; i < bufferLen; ++i) {
                int startCodeLen = 0;
                bool isStartCode = CheckStartCode(startCodeLen, buf+i, bufferLen-i);
                if(isStartCode){
                    endPos = i;
                    break;
                }
            }
            //找到了下一个起始码
            if(endPos > 0){
                //从buf[0:endPos] 是nalu,这里面包括起始码
                nalu.SetBuf(buffer, endPos);

                //以下代码是 去掉 取出出来的nalu
                uint8_t * _buffer = (uint8_t*)malloc(bufferLen - endPos);
                memcpy(_buffer, buffer + endPos, bufferLen - endPos);

                int new_bufferLen=bufferLen - endPos;

                freeBuffer();
                buffer = _buffer;
                bufferLen = new_bufferLen;


                return 0;
            } else{
                int readedLen = ReadFromFile();
                if(readedLen <= 0){//说明文件已经是结尾了
                    nalu.SetBuf(buffer, bufferLen);
                    freeBuffer();
                    return 1;
                }

            }
        }

    }

private:
    //判断 bufPtr 是否指向起始码
    bool CheckStartCode(int & startCodeLen, uint8_t * bufPtr, int bufLen){
        if(bufLen <= 2){
            startCodeLen = 0;
            return false;
        }
        //00 00 01
        if(bufPtr[0] == 0&&bufPtr[1] == 0&&bufPtr[2] == 1){
            startCodeLen = 3;
            return true;
        }
        //00 00 00 01
        if(bufLen >= 4&bufPtr[0] == 0&&bufPtr[1] == 0&&bufPtr[2] == 0&&bufPtr[3] == 1){
            startCodeLen = 4;
            return true;
        } else{
            startCodeLen = 0;
            return false;
        }

    }

    //从文件里面新读取了1024个字节，追加给buffer
    //返回读取到的字节数
    int ReadFromFile(){
        int tempBufferLen = 1024;
        uint8_t buf[tempBufferLen];

        int readedLen = fread(buf, 1, tempBufferLen, f);

        if(readedLen > 0){
            // 将新读取的 buf 添加到旧的 buffer 之后
            uint8_t * _buffer = (uint8_t *) malloc (bufferLen + readedLen);
            memcpy(_buffer,                 buffer, bufferLen);
            memcpy(_buffer + bufferLen,     buf,    readedLen);
            bufferLen = bufferLen + readedLen;

            if(buffer != nullptr){
                free(buffer);
            }

            buffer = _buffer;
        }


        return readedLen;
    }


    void freeBuffer(){
        if(buffer != nullptr){
            free(buffer);
        }
        buffer = nullptr;
        bufferLen = 0;
    }

    FILE * f;
    uint8_t * buffer;
    int bufferLen;
};

#endif //EYERLIB_EYER_AVC_BLOG_ANNEXBREADER_HPP