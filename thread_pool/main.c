#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *func(void *arg) {
  int num = *(int *)arg;
  printf("number is %d, thread %ld is working\n", num, pthread_self());
  sleep(1);
}

int main() {
  // create thread pool
  ThreadPool *pool = threadPoolCreate(3, 10, 100);
  for (int i = 0; i < 100; i++) {
    int *num = (int *)malloc(sizeof(int));
    *num = i + 100;
    threadPoolAdd(pool, func, num);
  }
  sleep(30);
  threadPoolDestroy(pool);

  return 0;
}
