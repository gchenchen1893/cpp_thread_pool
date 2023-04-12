#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

class Node {
public:
  int id;
  Node *next;
  Node(int id) {
    this->id = id;
    this->next = nullptr;
  }
};

pthread_cond_t cond;
pthread_mutex_t mutex;
Node *head;

// Producer
void *produce(void *arg) {
  while (true) {
    pthread_mutex_lock(&mutex);
    Node *node = new Node(rand() % 1000);
    node->next = head;
    head = node;
    std::cout << "Insert a new node, id: " << node->id
              << " thread id: " << pthread_self() << std::endl;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond);
    sleep(rand() % 3);
  }
  return NULL;
}

// Consumer
void *consume(void *arg) {
  while (true) {
    pthread_mutex_lock(&mutex);
    while (head == nullptr) {
      pthread_cond_wait(&cond, &mutex);
    }
    Node *node = head;
    head = head->next;
    std::cout << "Remove node, id: " << node->id
              << " thread id: " << pthread_self() << std::endl;
    delete node;
    pthread_mutex_unlock(&mutex);
    sleep(rand() % 3);
  }
  return NULL;
}

int main() {
  head = nullptr;
  pthread_cond_init(&cond, NULL);
  pthread_mutex_init(&mutex, NULL);
  pthread_t producer_tid[5];
  pthread_t consumer_tid[5];

  for (int i = 0; i < 5; i++) {
    pthread_create(&producer_tid[i], NULL, produce, NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_create(&consumer_tid[i], NULL, consume, NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(producer_tid[i], NULL);
  }

  for (int i = 0; i < 3; i++) {
    pthread_join(consumer_tid[i], NULL);
  }

  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&mutex);
  return 0;
}
