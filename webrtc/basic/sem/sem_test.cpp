#include <iostream>
#include <semaphore.h>

sem_t sem1;
int main() {
    int result= sem_init(&sem1,0,1);
    printf("result %d\n",result);


    sem_wait(&sem1);
    printf("wait 1\n");

    sem_wait(&sem1);
    printf("wait 2\n");
}