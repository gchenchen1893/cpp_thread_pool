#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int number = 0;

pthread_rwlock_t rwlock;

void *readNum(void *arg) {
  for (int i = 0; i < 50; i++) {
    pthread_rwlock_rdlock(&rwlock);
    std::cout << "[Read] index: " << i << " In thread A: " << pthread_self()
              << " nuber: " << number << std::endl;
    usleep(20);
    pthread_rwlock_unlock(&rwlock);
  }
  return NULL;
}

void *writeNum(void *arg) {
  for (int i = 0; i < 50; i++) {
    pthread_rwlock_wrlock(&rwlock);
    int cur = number;
    cur++;
    number = cur;
    std::cout << "[Write] index: " << i << " In thread B: " << pthread_self()
              << " number: " << number << std::endl;
    usleep(20);
    pthread_rwlock_unlock(&rwlock);
  }
  return NULL;
}

int main() {
  pthread_rwlock_init(&rwlock, NULL);
  pthread_t read_tid[5];
  pthread_t write_tid[3];

  for (int i = 0; i < 5; i++) {
    pthread_create(&read_tid[i], NULL, readNum, NULL);
  }

  for (int i = 0; i < 3; i++) {
    pthread_create(&write_tid[i], NULL, writeNum, NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(read_tid[i], NULL);
  }

  for (int i = 0; i < 3; i++) {
    pthread_join(write_tid[i], NULL);
  }

  pthread_rwlock_destroy(&rwlock);
  return 0;
}
