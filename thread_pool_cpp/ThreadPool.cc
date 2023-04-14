#include "ThreadPool.h"
#include "TaskQueue.h"
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <string>
#include <unistd.h>

static const int NUMBER = 2;

ThreadPool::ThreadPool(int minNum, int maxNum) {
  do {
    taskQ = new TaskQueue();
    if (taskQ == nullptr) {
      std::cout << "Failed to create task queue..." << std::endl;
      break;
    }
    threadIDs = new pthread_t[maxNum];
    if (threadIDs == nullptr) {
      std::cout << "Failed to create thread ids..." << std::endl;
      break;
    }
    memset(threadIDs, 0, sizeof(pthread_t) * maxNum);

    minNum = minNum;
    maxNum = maxNum;
    busyNum = 0;
    liveNum = minNum; // the same as the minimum threads number when
                      // constructing the object.
    exitNum = 0;

    if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
        pthread_cond_init(&notEmpty, NULL) != 0) {
      std::cout << "Failed to init lock..." << std::endl;
      break;
    }

    shutdown = false;

    // create threads
    pthread_create(&managerID, NULL, manager, this);
    for (int i = 0; i < minNum; i++) {
      // keep grab the task node and execute it.
      pthread_create(&threadIDs[i], NULL, worker, this);
    }
    return;
  } while (0);

  // Capture exception and then release resources.
  if (threadIDs) {
    delete[] threadIDs;
  }
  if (taskQ) {
    delete taskQ;
  }
}

ThreadPool::~ThreadPool() {
  pthread_mutex_lock(&mutexPool);
  shutdown = true;
  pthread_mutex_unlock(&mutexPool);

  pthread_join(managerID, NULL);
  for (int i = 0; i < liveNum; i++) {
    pthread_cond_signal(&notEmpty);
  }

  // release stack memory
  if (taskQ) {
    delete taskQ;
  }

  if (threadIDs) {
    delete threadIDs;
  }

  // release locks
  pthread_mutex_destroy(&mutexPool);
  pthread_cond_destroy(&notEmpty);
}

// 1. Insert new task
void ThreadPool::AddTask(const Task &task) {
  if (shutdown == false) {
    taskQ->addTask(task);
    // wake up blocking consumers.
    pthread_cond_signal(&notEmpty);
  }
}

// 2. Fetch the busy number of thread pool
int ThreadPool::getBusyNum() {
  pthread_mutex_lock(&mutexPool);
  int busyNum_res = busyNum;
  pthread_mutex_unlock(&mutexPool);
  return busyNum_res;
}

// 3. Fetch the living number of thread pool
int ThreadPool::getAliveNum() {
  pthread_mutex_lock(&mutexPool);
  int aliveNum_res = liveNum;
  pthread_mutex_unlock(&mutexPool);
  return aliveNum_res;
}

void *ThreadPool::worker(void *arg) {
  ThreadPool *pool = static_cast<ThreadPool *>(arg);

  while (true) {
    // lock the whole pool
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->taskQ->taskNumber() == 0 && pool->shutdown == false) {
      pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

      if (pool->exitNum > 0) {
        pool->exitNum--;
        if (pool->liveNum > pool->minNum) {
          pool->liveNum--;
          // have to release the lock since wait() will release the 1st lock and
          // then obtain the second lock.
          pthread_mutex_unlock(&pool->mutexPool);
          pool->threadExit();
        }
      }
    }

    // ensure the pool is not shutdown.
    if (pool->shutdown == true) {
      pthread_mutex_unlock(&pool->mutexPool);
      pool->threadExit();
    }

    // fetch the task from the queue.
    Task task = pool->taskQ->takeTask();

    // update the busy num + 1
    pool->busyNum++;

    // unlock the pool
    // wake up the producer
    pthread_mutex_unlock(&pool->mutexPool);

    // execute the task
    std::cout << "thread " << pthread_self() << " starts working..."
              << std::endl;
    task.function(task.arg);
    free(task.arg);
    task.arg = nullptr;

    // update the busy num - 1
    std::cout << "thread " << pthread_self() << " stops working..."
              << std::endl;
    pthread_mutex_lock(&pool->mutexPool);
    pool->busyNum--;
    pthread_mutex_unlock(&pool->mutexPool);
  }

  return NULL;
}

void *ThreadPool::manager(void *arg) {
  ThreadPool *pool = static_cast<ThreadPool *>(arg);
  while (pool->shutdown == false) {
    // monitor every 3 seconds.
    sleep(3);

    // fetch current task number and working thread number.
    // fetch current busy thread number.
    pthread_mutex_lock(&pool->mutexPool);
    int queueSize = pool->taskQ->taskNumber();
    int liveNum = pool->liveNum;
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexPool);

    // add new threads (2 threads each iteration).
    // task number > liveNum and liveNum < maxNum
    if (queueSize > liveNum && liveNum < pool->maxNum) {
      pthread_mutex_lock(&pool->mutexPool);
      int counter = 0;
      for (int i = 0;
           i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum;
           i++) {
        if (pool->threadIDs[i] == 0) {
          pthread_create(&pool->threadIDs[i], NULL, worker, pool);
          counter++;
          pool->liveNum++;
        }
      }
      pthread_mutex_unlock(&pool->mutexPool);
    }

    // remove threads (2 threads each iteration).
    // more than 50% threads are not busy and at least minNum live threads.
    if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
      pthread_mutex_lock(&pool->mutexPool);
      pool->exitNum = NUMBER;
      pthread_mutex_unlock(&pool->mutexPool);

      // make thread exit it self when exitNum > 0.
      for (int i = 0; i < NUMBER; i++) {
        pthread_cond_signal(&pool->notEmpty);
      }
    }
  }

  pthread_exit(NULL);
  return NULL;
}

// exit thread and help reset the thread id to 0.
void ThreadPool::threadExit() {
  pthread_t tid = pthread_self();

  for (int i = 0; i < maxNum; i++) {
    if (threadIDs[i] == tid) {
      threadIDs[i] = 0;
      std::cout << "Exit " << pthread_self() << std::endl;
      break;
    }
  }
  pthread_exit(NULL);
}
