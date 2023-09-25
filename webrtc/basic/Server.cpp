#include "Server.h"
#include <iostream>
#include "unistd.h"

namespace rtcbase{
    Server::Server(){}
    Server::~Server(){}

    void Server::Run() {
        while (1){
            std::cout<<"running..."<<std::endl;
            usleep(1e6);
        }
    }
}