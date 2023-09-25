#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
/*
 * 本程序多执行几次会发生死锁，原因如下
 * 1.printf,puts执行都需要先对控制台加锁。
 * 2.在main执行printf的时候，有大量子进程退出。 主进程会收到SIGCHLD中断。
 * 3.如果中断正好打断printf执行，锁没有是否，终端程序catch_child的puts也无法获得锁，最终导致死锁。
 */
#define DEADLOCK 1


static void catch_child(int signo)
{
#if DEADLOCK
    /* this call may reenter printf/puts! Bad! */
    puts("Child exitted!\n");
#else
    /* This version is async-signal-safe */
    sio_puts("Child exitted!\n");
#endif

    /* reap all children */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;
}

int main()
{
    signal(SIGCHLD, catch_child);

    long i, j;
    for (i = 0; i < 1000000; i++) {
        if (fork() == 0)
            exit(0);

#if DEADLOCK
        printf("Child started\n");
#else
        sio_puts("Child started\n");
#endif

        for (j=0; j<10; j++) {
#if DEADLOCK
            /* Make each printf do a lot of work */
            printf("%ld%ld%ld%ld%ld%ld%ld%ld", i, i, i, i, i, i, i, i);
#else
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
            sio_emitl("i");
#endif
        }
    }
    return 0;
}