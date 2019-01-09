#include <stdio.h>
#include <unistd.h>
#include "threadpool.cpp"


void* mytask(void *arg)
{
    printf("thread %d is working on task %d\n", (int)pthread_self(), *(int*)arg);
    sleep(1);
    free(arg);
    return NULL;
}

int main() {
    struct threadpool* pool;
    pool=threadpool_init(4);
    int i;
    //创建十个任务
    for(i=0; i < 10; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        threadpool_add(pool, mytask, arg);
    }
    sleep(10);
        return 0;
}