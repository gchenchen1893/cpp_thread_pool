#ifndef _THREADPOOL_H
#define _THREADPOOL_H

typedef struct ThreadPool ThreadPool;
// 1. Create and initialize a thread pool
ThreadPool *threadPoolCreate(int min /* minNum */, int max /* maxNum */,
                             int queueSize /* queue capacity */);
void *worker(void *arg);
void *manager(void *arg);
void threadExit(ThreadPool *pool);

// 2. Destroy the thread pool
int threadPoolDestroy(ThreadPool* pool);

// 3. Insert new task
void threadPoolAdd(ThreadPool *pool, void (*func)(void *), void *arg);

// 4. Fetch the busy number of thread pool
int threadPoolBusyNum(ThreadPool* pool);

// 5. Fetch the living number of thread pool
int threadPoolAliveNum(ThreadPool* pool);

#endif // _THREADPOOL_H
