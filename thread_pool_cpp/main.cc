#include "TaskQueue.h"
#include "ThreadPool.h"
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

void func(void *arg) {
  int num = *(int *)arg;
  std::cout << "number is " << num << " thread " << pthread_self()
            << "is working" << std::endl;
  sleep(1);
}

int main() {
  // create thread pool
  ThreadPool pool(3, 10);

  for (int i = 0; i < 100; i++) {
    int *num = new int(i + 100);
    pool.AddTask(Task(func, (void*)num));
  }

  sleep(20);

  return 0;
}
