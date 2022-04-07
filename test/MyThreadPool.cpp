#include "MyThreadPool.h"
#include "muduo/base/Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string &nameArg)
        : name_(nameArg),
          maxQueueSize_(0),
          running_(false),
          mutex_(),
          notEmpty_(mutex_),
          notFull_(mutex_) {

}

ThreadPool::~ThreadPool() {
    if (running_) {
        stop();
    }
}

// 线程池初始化 主要是线程队列初始化
void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    // running_
    running_ = true;
    // 为threads_线程队列预留空间
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i + 1);
        // 创建线程(muduo::Thread类的一个对象)并添加到线程队列中 -- 实际线程创建在muduo::Thread::start()函数中
        // 创建的线程绑定的函数为runInThread() 从任务队列中取出一个任务
        threads_.emplace_back(new muduo::Thread(
                std::bind(&ThreadPool::runInThread, this), name_+id
                ));
        // 启动线程
        // 实际是muduo::Thread::start()函数负责创建新线程 并绑定runInThread()函数
        threads_[i]->start();
    }

    // 未创建新线程 则使用当前主线程
    if (numThreads == 0 && threadInitCallback_) {
        threadInitCallback_();
    }

}

void ThreadPool::stop() {
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notify();
        notFull_.notify();
    }
    for (auto& thr : threads_) { thr->join(); }
}

size_t ThreadPool::queueSize() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

// 生产者进程
void ThreadPool::run(Task task) {
    // 如果当前线程队列中没有线程(当前没有消费者) 则由ThreadPool直接代为执行该任务
    if (threads_.empty()) {
        task();
    } else {
        // 生产者放产品的过程
        MutexLockGuard lock(mutex_);
        while (isFull() && running_) {
            notFull_.wait();
        }
        if (!running_) return;
        assert(!isFull());

        // 添加任务到任务队列
        queue_.push_back(std::move(task));
        notEmpty_.notify();
    }
}

// 消费者
ThreadPool::Task ThreadPool::take() {
    // 取产品的过程
    MutexLockGuard lock(mutex_);
    // 任务队列空且线程池处于运行状态
    while (queue_.empty() && running_) {
        notEmpty_.wait();
    }
    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0) {
            notFull_.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull() const {
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

// 线程队列中线程绑定的函数
void ThreadPool::runInThread() {
    try {
        if (threadInitCallback_) {
            threadInitCallback_();
        }
        while (running_) {
            // 从任务队列中取出任务并执行任务
            // 取任务
            Task task(take());// take()返回任务队列中的任务函数--一个函数指针/地址
            // 执行任务
            if (task) {
                task();// 运行函数
            }
        }
    } catch (const Exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}