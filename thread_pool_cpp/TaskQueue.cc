#include "TaskQueue.h"
#include "pthread.h"

TaskQueue::TaskQueue() { pthread_mutex_init(&m_mutex, NULL); }

TaskQueue::~TaskQueue() { pthread_mutex_destroy(&m_mutex); }

// Add a task
void TaskQueue::addTask(const Task &task) {
  pthread_mutex_lock(&m_mutex);
  m_taskQ.push(task);
  pthread_mutex_unlock(&m_mutex);
}

void TaskQueue::addTask(callback f, void *arg) {
  pthread_mutex_lock(&m_mutex);
  m_taskQ.push(Task(f, arg));
  pthread_mutex_unlock(&m_mutex);
}

// Fetch a task
Task TaskQueue::takeTask() {
  Task task(nullptr, nullptr);
  pthread_mutex_lock(&m_mutex);
  if (!m_taskQ.empty()) {
    task = m_taskQ.front();
    m_taskQ.pop();
  }
  pthread_mutex_unlock(&m_mutex);
  return task;
}
