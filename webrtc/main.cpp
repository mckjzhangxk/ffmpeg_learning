#include <iostream>
#include "basic/Server.h"
int main() {
    rtcbase::Server* server=new rtcbase::Server();

    server->Run();
    return 0;
}
