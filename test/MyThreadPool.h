#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Types.h"

#include <deque>
#include <vector>

namespace muduo {

class ThreadPool : noncopyable {
public:
    typedef std::function<void ()> Task;

    explicit ThreadPool(const string& nameArg = string("ThreadPool"));
    ~ThreadPool();

    // 在start()之前调用
    void setMaxQueueSize(int maxSize) {maxQueueSize_ = maxSize;}// 任务队列中任务的数量
    void setThreadInitCallback(const Task& cb) {threadInitCallback_ = cb;}

    // 线程池初始化
    // numThreads：启动的线程个数
    void start(int numThreads);
    void stop();

    const string& name() const {return name_;}
    size_t queueSize() const;

    // 向任务队列中添加任务 生产者
    void run(Task f);

private:
    bool isFull() const REQUIRES(mutex_);// 判断任务队列是否为满 queue_.size >= maxQueueSize?
    void runInThread();// 调用take()函数实现从线程池中取出任务 消费者
    Task take();// 去线程池中取任务 消费者

    string name_;// 线程池名称
    size_t maxQueueSize_;// 任务队列中任务的数量
    bool running_;// 标志线程池是否处于运行状态

    mutable MutexLock mutex_;
    Condition notEmpty_ GUARDED_BY(mutex_);
    Condition notFull_ GUARDED_BY(mutex_);
    std::deque<Task> queue_ GUARDED_BY(mutex_);// 任务队列 缓冲区
    std::vector<std::unique_ptr<muduo::Thread>> threads_;// 线程队列 消费者
    Task threadInitCallback_;
};

}// namespace muduo

#endif