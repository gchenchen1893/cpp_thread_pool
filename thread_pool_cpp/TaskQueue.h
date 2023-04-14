#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <pthread.h>
#include <queue>

using callback = void(*)(void*);

class Task {
public:
  Task(callback func, void *arg) {
    this->arg = arg;
    function = func;
  }

  callback function;
  void *arg;
};

class TaskQueue {
public:
  TaskQueue();
  ~TaskQueue();

  // Add a task
  void addTask(const Task &task);
  void addTask(callback f, void *arg);

  // Fetch a task
  Task takeTask();

  // Get current task queue size
  inline int taskNumber() { return m_taskQ.size(); }

private:
  pthread_mutex_t m_mutex;
  std::queue<Task> m_taskQ;
};

#endif // _TASKQUEUE_H_
