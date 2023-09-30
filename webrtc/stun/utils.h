#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
int createUDPSocket(const char * ip,int port);
void sendMsgTo(int socketFd, struct sockaddr_in &remote_addr,std::string&msg);
void sendMsgTo(int socketFd,const char * ip,int port,std::string&msg);
std::string recvMsg(int sockFd);