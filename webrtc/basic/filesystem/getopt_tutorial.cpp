#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int parse_attack_cmd(int argc, char *argv[]){
    int opt; //表示选项
    long time=-1;
    //这里支持t=v 的选项,:表示后面的valye
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                time= atol(optarg); //optarg表示选择的参数
                break;
            case '?':
                fprintf(stderr, "Unknown option character '-%c'.\n", optopt);
                return -1;
            default:
                break;
        }
    }

    int index=optind&0xff;
    if (index>argc-2){
        return -2;
    }
    char *src=argv[optind]; //optind表示解析完成后，下一个将被处理的 argv 索引。
    char *dest=argv[optind+1];

    // 打印解析的选项值

    printf("time: %ld\n", time);

    // 使用 sscanf 分隔 IP 地址和端口
    char src_ip[15]; // Assuming IPv4 address (xxx.xxx.xxx.xxx) with null terminator
    int src_port;

    if (sscanf(src, "%[^:]:%d", src_ip, &src_port) == 2) {
        printf("IP Address: %s\n", src_ip);
        printf("Port: %d\n", src_port);
    } else {
        printf("Invalid input format\n");
    }

    char dest_ip[15]; // Assuming IPv4 address (xxx.xxx.xxx.xxx) with null terminator
    int dest_port;

    if (sscanf(dest, "%[^:]:%d", dest_ip, &dest_port) == 2) {
        printf("IP Address: %s\n", dest_ip);
        printf("Port: %d\n", dest_port);
    } else {
        printf("Invalid input format\n");
    }

}
int main(int argc, char *argv[]) {
    char *subcmd=NULL;

    if (argc<2){
        printf("usage attack subcmd [option] ...\n");
        return EXIT_FAILURE;
    }
    // 使用getopt解析命令行选项,解析-i -o
    //attack syn -t 300 7.7.7.7:30 8.8.8.8:50
    subcmd=argv[1];
    printf("subcmd:%s\n",subcmd);
    argc--;
    argv++;

    if (0== strcmp(subcmd,"sync")){
        parse_attack_cmd(argc,argv);
    }

    return EXIT_SUCCESS;
}
