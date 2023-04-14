#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "TaskQueue.h"
#include <pthread.h>

class ThreadPool {
public:
  ThreadPool(int minNum, int maxNum);
  ~ThreadPool();

  // 1. Insert new task
  void AddTask(const Task &task);

  // 2. Fetch the busy number of thread pool
  int getBusyNum();

  // 3. Fetch the living number of thread pool
  int getAliveNum();

private:
  static void *worker(void *arg);
  static void *manager(void *arg);
  void threadExit();

  TaskQueue *taskQ;

  pthread_t managerID;
  pthread_t *threadIDs; // a list of working thread
  int minNum;           // minumum number of threads
  int maxNum;
  int busyNum; // working threads
  int liveNum; // working + ready threads
  int exitNum; // killing number of threads

  pthread_mutex_t mutexPool; // lock the whole pool, task queue
  pthread_cond_t notEmpty;   // is taskqueue empty

  static const int NUMBER = 2;
  bool shutdown; // either destroy the whole pool
};

#endif // _THREADPOOL_H_
