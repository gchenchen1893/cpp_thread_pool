#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int NUMBER = 2;

typedef struct Task {
  void (*function)(void *args);
  void *arg;
} Task;

struct ThreadPool {
  Task *taskQueue;
  int queueCapacity; // current number of tasks
  int queueSize;     // max number of tasks
  int queueFront;
  int queueRear;

  pthread_t managerID;
  pthread_t *threadIDs; // a list of working thread
  int minNum;           // minumum number of threads
  int maxNum;
  int busyNum; // working threads
  int liveNum; // working + ready threads
  int exitNum; // killing number of threads

  pthread_mutex_t mutexPool; // lock the whole pool, task queue
  pthread_mutex_t mutexBusy; // lock busyNum;
  pthread_cond_t notFull;    // is taskqueue full
  pthread_cond_t notEmpty;   // is taskqueue empty

  int shutdown; // either destroy the whole pool
};

ThreadPool *threadPoolCreate(int min /* minNum */, int max /* maxNum */,
                             int queueSize /* queue capacity */) {
  ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
  do {
    if (pool == NULL) {
      printf("Failed to malloc pool...\n");
      break;
    }

    pool->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * max);
    if (pool->threadIDs == NULL) {
      printf("Failed to malloc thread ids...\n");
      break;
    }
    memset(pool->threadIDs, 0, sizeof(pthread_t) * max);

    pool->minNum = min;
    pool->maxNum = max;
    pool->busyNum = 0;
    pool->liveNum = min; // the same as the minimum threads number when
                         // constructing the object.
    pool->exitNum = 0;

    if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
        pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
        pthread_cond_init(&pool->notFull, NULL) != 0 ||
        pthread_cond_init(&pool->notEmpty, NULL) != 0) {
      printf("Failed to init lock...\n");
      break;
    }

    // task queue
    pool->taskQueue = (Task *)malloc(sizeof(Task) * queueSize);
    if (pool->taskQueue == NULL) {
      printf("Failed to init taskQueue...\n");
      break;
    }
    pool->queueCapacity = queueSize;
    pool->queueSize = 0;
    pool->queueFront = 0;
    pool->queueRear = 0;

    pool->shutdown = 0;

    // create threads
    pthread_create(&pool->managerID, NULL, manager, pool);
    for (int i = 0; i < min; i++) {
      // keep grab the task node and execute it.
      pthread_create(&pool->threadIDs[i], NULL, worker, pool);
    }

    return pool;

  } while (0);

  // Capture exception and then release resources.
  if (pool && pool->threadIDs) {
    free(pool->threadIDs);
  }

  if (pool && pool->taskQueue) {
    free(pool->taskQueue);
  }

  if (pool) {
    free(pool);
  }

  return NULL;
}

void *worker(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  while (1) {
    // lock the whole pool
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == 0 && pool->shutdown == 0) {
      pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
      if (pool->exitNum > 0) {
        pool->exitNum--;
        if (pool->liveNum > pool->minNum) {
          pool->liveNum--;
          // have to release the lock since wait() will release the 1st lock and
          // then obtain the second lock.
          pthread_mutex_unlock(&pool->mutexPool);
          threadExit(pool);
        }
      }
    }

    // ensure the pool is not shutdown.
    if (pool->shutdown == 1) {
      pthread_mutex_unlock(&pool->mutexPool);
      threadExit(pool);
    }

    // fetch the task from the queue.
    Task task;
    task.function = pool->taskQueue[pool->queueFront].function;
    task.arg = pool->taskQueue[pool->queueFront].arg;

    // update task queue size.
    pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
    pool->queueSize--;

    // unlock the pool
    // wake up the producer
    pthread_cond_signal(&pool->notFull);
    pthread_mutex_unlock(&pool->mutexPool);

    // update the busy num + 1
    pthread_mutex_lock(&pool->mutexBusy);
    pool->busyNum++;
    pthread_mutex_unlock(&pool->mutexBusy);

    // execute the task
    printf("thread %ld is working...\n", pthread_self());
    task.function(task.arg);
    free(task.arg);
    task.arg = NULL;

    // update the busy num - 1
    printf("thread %ld stops working...\n", pthread_self());
    pthread_mutex_lock(&pool->mutexBusy);
    pool->busyNum--;
    pthread_mutex_unlock(&pool->mutexBusy);
  }

  return NULL;
}

void *manager(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  while (pool->shutdown == 0) {
    // monitor every 3 seconds.
    sleep(3);

    // fetch current task number and working thread number.
    pthread_mutex_lock(&pool->mutexPool);
    int queueSize = pool->queueSize;
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);

    // fetch current busy thread number.
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);

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
void threadExit(ThreadPool *pool) {
  pthread_t tid = pthread_self();

  for (int i = 0; i < pool->maxNum; i++) {
    if (pool->threadIDs[i] == tid) {
      printf("Exit %ld thread...\n", tid);
      pool->threadIDs[i] = 0;
      break;
    }
  }
  pthread_exit(NULL);
}

void threadPoolAdd(ThreadPool *pool, void (*func)(void *), void *arg) {
  pthread_mutex_lock(&pool->mutexPool);
  while (pool->queueSize == pool->queueCapacity && pool->shutdown == 0) {
    pthread_cond_wait(&pool->notFull, &pool->mutexPool);
  }
  if (pool->shutdown == 1) {
    pthread_mutex_unlock(&pool->mutexPool);
    return;
  }

  pool->taskQueue[pool->queueRear].function = func;
  pool->taskQueue[pool->queueRear].arg = arg;
  pool->queueSize++;
  pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;

  // wake up blocking consumers.
  pthread_cond_signal(&pool->notEmpty);
  pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool *pool) {
  pthread_mutex_lock(&pool->mutexBusy);
  int busyNum = pool->busyNum;
  pthread_mutex_unlock(&pool->mutexBusy);
  return busyNum;
}

int threadPoolAliveNum(ThreadPool *pool) {
  pthread_mutex_lock(&pool->mutexPool);
  int aliveNum = pool->liveNum;
  pthread_mutex_unlock(&pool->mutexPool);
  return aliveNum;
}

int threadPoolDestroy(ThreadPool *pool) {
  if (pool == NULL) {
    return -1;
  }

  pthread_mutex_lock(&pool->mutexPool);
  pool->shutdown = 1;
  pthread_mutex_unlock(&pool->mutexPool);

  pthread_join(&pool->managerID, NULL);
  for (int i = 0; i < pool->liveNum; i++) {
    pthread_cond_signal(&pool->notEmpty);
  }

  // release stack memory
  if (pool->taskQueue) {
    free(pool->taskQueue);
  }

  if (pool->threadIDs) {
    free(pool->threadIDs);
  }

  // release locks
  pthread_mutex_destroy(&pool->mutexPool);
  pthread_mutex_destroy(&pool->mutexBusy);
  pthread_cond_destroy(&pool->notFull);
  pthread_cond_destroy(&pool->notEmpty);

  free(pool);
  pool = NULL;
  return 0;
}
