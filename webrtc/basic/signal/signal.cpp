#include <stdio.h>
#include <unistd.h>
#include <signal.h>
void sigint_handler(int sig) /* SIGINT handler */
{
    printf("捕捉到信号 %d\n",sig);
}
// kill -SIGQUIT  59863, CTRL+\
// kill -SIGINIT  59863, CTRL+C

/**
 * SIGPIPE 信号是读取pipe的一端关闭会得到本信号
 * SIGCHLD 是子进程退出会获得此信号
 * SIGHUP 是父进程退出会获得此信号
 * */
int main()
{
    printf("pid %d\n",getpid());
    /* Install the SIGINT handler */
    signal(SIGINT, sigint_handler);
    signal(SIGHUP, sigint_handler);
    signal(SIGQUIT, sigint_handler);

    /* Wait for the receipt of a signal */
    while (1){
        pause();
    }


    return 0;
}