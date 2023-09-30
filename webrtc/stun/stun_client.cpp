#include "utils.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#define MESSAGE_LENGTH 512*4


//127.0.0.1 9999 hello
//127.0.0.1 9998 hello

//91.245.255.31 9999 hello
//91.245.255.31 9998 hello
int main(int argc, char * *argv)
{
    if (argc<2){
        cout<<"Usage: localport"<<endl;
        return 0;
    }
    //////////////////////////////创建套接字///////////////////////////
    int sockFd= createUDPSocket("0.0.0.0", atoi(argv[1]));


    char ip[64];
    int port;
    std::string msg;

    pid_t pid=fork();

    while (true){
        if (pid>0){
            cout<<"输入远端的ip port:";
            cin>>ip>>port>>msg;

            sendMsgTo(sockFd,ip,port,msg);
        }else{
            cout<<recvMsg(sockFd)<<endl;
        }
    }



}

