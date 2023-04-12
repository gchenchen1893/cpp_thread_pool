#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int number = 0;

pthread_mutex_t mutex;

void *callbackA(void *arg) {
  for (int i = 0; i < 50; i++) {
    pthread_mutex_lock(&mutex);
    int cur = number;
    cur++;
    usleep(20);
    number = cur;
    std::cout << "index: " << i << " In thread A: " << pthread_self()
              << " nuber: " << number << std::endl;
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void *callbackB(void *arg) {
  for (int i = 0; i < 50; i++) {
    pthread_mutex_lock(&mutex);
    int cur = number;
    cur++;
    usleep(20);
    number = cur;
    std::cout << "index: " << i << " In thread B: " << pthread_self()
              << " number: " << number << std::endl;
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main() {
  pthread_mutex_init(&mutex, NULL);
  pthread_t tidA;
  pthread_t tidB;
  pthread_create(&tidA, NULL, callbackA, NULL);
  pthread_create(&tidB, NULL, callbackB, NULL);
  pthread_join(tidA, NULL);
  pthread_join(tidB, NULL);
  pthread_mutex_destroy(&mutex);
  return 0;
}
