#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

typedef struct Box{
    char * content;
    long content_position;
    char type[4];
    u_int64_t size;//8+sizeof(content)
    int level;

    int parent_next_position;
}Box;
void parse_mvhd(Box* box,FILE*fp){
    if(strcmp(box->type,"mvhd")!=0)
        return;
    fseek(fp,12,SEEK_CUR);

    u_int32_t timescale=0;
    u_int32_t duration=0;

    fread(&timescale,1,4,fp);
    fread(&duration,1,4,fp);

    timescale= htonl(timescale);
    duration= htonl(duration);

    printf("[timescale:%d,duration:%d]\n",timescale,duration);
    fseek(fp,box->content_position,SEEK_SET);
}

void parse_tkhd(Box* box,FILE*fp){
    if(strcmp(box->type,"tkhd")!=0)
        return;
    fseek(fp,12,SEEK_CUR);

    u_int32_t trackId=0;
    u_int32_t width=0,height=0;

    fread(&trackId,1,4,fp);
    fseek(fp,60,SEEK_CUR);
    fread(&width,1,4,fp);
    fread(&height,1,4,fp);

    trackId= htonl(trackId);
    width= htonl(width)>>16; //16.16浮点表示 1000000000000000.1000000000000000
    height= htonl(height)>>16;

    printf("[trackId:%d,width:%d height:%d]\n",trackId,width,height);
    fseek(fp,box->content_position,SEEK_SET);
}

void parse_mdhd(Box* box,FILE*fp){
    if(strcmp(box->type,"mdhd")!=0)
        return;
    fseek(fp,12,SEEK_CUR);

    u_int32_t timescale=0;
    u_int32_t duration=0;

    fread(&timescale,1,4,fp);
    fread(&duration,1,4,fp);

    timescale= htonl(timescale);
    duration= htonl(duration);

    printf("[timescale:%d,duration:%d]\n",timescale,duration);
    fseek(fp,box->content_position,SEEK_SET);
}

void parse_hdlr(Box* box,FILE*fp){
    if(strcmp(box->type,"hdlr")!=0)
        return;
    fseek(fp,8,SEEK_CUR);

    char handle[5]={0};


    fread(handle,1,4,fp);


    printf("[handle type %s]\n",handle);
    fseek(fp,box->content_position,SEEK_SET);
}
void printBox(Box * box,long line,FILE*fp){
    for (int i = 0; i < box->level; ++i) {
        printf("   ");
    }
    printf("%010ld %s 0x%x\n",line,box->type,box->size);
    parse_mvhd(box,fp);
    parse_tkhd(box,fp);
    parse_mdhd(box,fp);
    parse_hdlr(box,fp);
}
//返回值是读取的位置: len  type content
//文件指针停留在了读完 len type的位置
//box.size是上一个平级读取的box的大小
long read_next_box(Box * box,FILE * fp){
    if(box->size){
        fseek(fp,box->size-8,SEEK_CUR);
    } else{
        fseek(fp,0,SEEK_CUR);
    }

    int n=fread(&box->size,1,4,fp);
    if(n<4){
        return -1;
    }
    box->size=(box->size&0xff)<<24 |(box->size&0xff00)<<8|(box->size&0xff0000)>>8|(box->size&0xff000000)>>24;

    n=fread(box->type,1,4,fp);
    if(n<4){
        return -1;
    }
    box->content_position=ftell(fp);

    //我的position不可能是我parent_next_position,我们根本不同级
    if(box->parent_next_position == (box->content_position - 8) ){
        fseek(fp,-8,SEEK_CUR);
        return -1;
    }
    return box->content_position-8;
}


//返回值是读取的位置: len  type content
//输入的文件指针一定是停在读取content的地方
long read_inner_box(Box*box,FILE *fp){
    int n=fread(&box->size,1,4,fp);
    if(n<4){
        return -1;
    }
    box->size=(box->size&0xff)<<24 |(box->size&0xff00)<<8|(box->size&0xff0000)>>8|(box->size&0xff000000)>>24;

    n=fread(box->type,1,4,fp);
    if(n<4){
        return -1;
    }
    box->content_position=ftell(fp);
    return box->content_position-8;
}

bool hasChildBox(char* type){
    if(strcmp("mvhd",type)==0){
        return false;
    }
    if(strcmp("moov",type)==0){
        return true;
    }
    if(strcmp("trak",type)==0){
        return true;
    }
    if(strcmp("mdia",type)==0){
        return true;
    }
    if(strcmp("minf",type)==0){
        return true;
    }
    if(strcmp("stbl",type)==0){
        return true;
    }

    return false;
}

void cpybox(Box* src,Box* dst){
    dst->size=src->size;
    dst->content_position=src->content_position;
    dst->level=src->level;
    dst->parent_next_position=src->parent_next_position;
    strncpy(dst->type,src->type,4);
}


void mp4_struct_parser(Box*box,FILE *fp){
    while (true){
        int n=read_next_box(box,fp);
        if(n<0)
            break;
        printBox(box,n,fp);
        if(hasChildBox(box->type)){
            Box tmp;
            cpybox(box,&tmp);

            n=read_inner_box(box,fp);
            if(n>=0){
                box->level=tmp.level+1;
                box->parent_next_position= tmp.content_position + tmp.size - 8;
                printBox(box,n,fp);
                mp4_struct_parser(box,fp);
            }


            cpybox(&tmp,box);
            fseek(fp,box->content_position,SEEK_SET);
        }
//        if(strcmp("edts",box->type)==0){
//            printf("xxxx\n");
//        }
    }
}
int main(int argc, char *argv[]) {
    Box box={0};

    FILE *fp=fopen("/Users/zhanggxk/Desktop/测试视频/jojo.mp4","rb");

    box.parent_next_position=-1;
    mp4_struct_parser(&box,fp);






    fclose(fp);
    return 0;
}