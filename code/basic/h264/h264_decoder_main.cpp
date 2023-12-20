#include "AnnexBReader.hpp"
#include "NaluSPS.hpp"
#include "NaluPPS.hpp"
#include "BitStream.hpp"
int main(int argc, char const *argv[]){
    std::string filePath = "./test1.h264";
    AnnexBReader reader(filePath);

    int i=0;
    while(1){
        Nalu nalu;
        int isLast = reader.ReadNalu(nalu);


        nalu.ParseHeader();
        nalu.ParseRBSP();

//        printf("[%5d] Nalu Type: %d,Size %d\n", i++,nalu.GetNaluType(),nalu.GetSize());
        if(isLast){
            break;
        }
        if (nalu.GetNaluType()==7){//sps
            NaluSPS sps(nalu);
            sps.Parse();
            int width=16*(sps.pic_width_in_mbs_minus1+1);
            //场编码，一帧等于两场,frame_mbs_only_flag=1表示帧编码，,frame_mbs_only_flag=1表示场编码，
            int height=16*(sps.pic_height_in_map_units_minus1+1)*(2-sps.frame_mbs_only_flag); //


            if(sps.frame_cropping_flag){
                int xunit,yubit;

                if (sps.chroma_format_idc==0){
                    xunit=1;
                    yubit=1;
                } else if (sps.chroma_format_idc==1){
                    xunit=2;
                    yubit=2;
                } else if (sps.chroma_format_idc==2){
                    xunit=2;
                    yubit=1;
                } else if (sps.chroma_format_idc==3){
                    xunit=1;
                    yubit=1;
                }
                width-=sps.frame_crop_left_offset+sps.frame_crop_right_offset*xunit;
                height-=sps.frame_crop_top_offset+sps.frame_crop_bottom_offset*yubit*(2-sps.frame_mbs_only_flag);//考虑场编码
            }

            printf("profile[%d]:%dx%d\n",sps.profile_idc,width,height);
        }else if(nalu.GetNaluType()==8){
            NaluPPS pps(nalu);
            pps.Parse();

            printf("pps id:%d %d\n",pps.num_ref_idx_l0_active_minus1,pps.num_ref_idx_l1_active_minus1);
        }

    }
    return 0;
}