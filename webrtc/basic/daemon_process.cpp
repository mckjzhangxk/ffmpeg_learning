#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <string>
//后台运行程序的实现步骤
//1 fork()子进程，父进程退出
//2. 子进程设置新的session id
//3. 子进程设置新的工作目录[可选]
//4. 子进程重定向标准IO[可选]

//以下等价于调用C库提供的daemon方法
void daemonize(){

    if (fork()){
        exit(0);
    }

    //子进程执行到此
    std::cout<<"child pid "<<getpid()<<std::endl;
    setsid();   //重新设置 session id


    chdir("/"); //设置当前工作目录

    //重定向stderr,stdout,stdin

    //我想重定向到其他文件，行不通，为什么？
//    char current_dir[128];
//    getcwd(current_dir,128);
//    std::string std_path=current_dir;
//    std_path+="/"+std::to_string(getpid())+".pid";

    std::string std_path="/dev/null";
    int fd= open(std_path.c_str(),O_RDWR);

    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);

//    dup2(STDOUT_FILENO,fd);
//    dup2(STDERR_FILENO,fd);
}
//ps -ef|grep daemon_process
int main(){
    daemonize();
    // daemon(int nochdir, int noclose);
    daemon(0,0); //chdir -> /,std->/dev/null

    while (1){
        std::cout<<"running..."<<std::endl;
        usleep(3e6);
    }
}