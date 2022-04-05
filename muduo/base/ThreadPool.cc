// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/ThreadPool.h"

#include "muduo/base/Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  // running_置为true
  running_ = true;
  // 为threads_线程队列预置空间
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i+1);
    // 创建线程(muduo::Thread类的一个对象)并添加到线程队列中
    // 创建的线程绑定的函数为runInThread()
    threads_.emplace_back(new muduo::Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));// name_+id 线程名称
    // 启动线程
    // muduo::Thread类的start()函数负责创建线程
    threads_[i]->start();
  }
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  // 将running_置为false
  running_ = false;
  notEmpty_.notifyAll();
  notFull_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();// 等待线程的退出
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(Task task)
{
  // 如果当前线程队列中没有线程(当前没有消费者) 则由ThreadPool直接代为执行该任务
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull() && running_)
    {
      notFull_.wait();
    }
    if (!running_) return;
    assert(!isFull());

    // 添加任务task到任务队列
    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);// 保护队列
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_)// 任务队列空且线程池处于运行状态
  {
    notEmpty_.wait();
  }
  Task task;
  if (!queue_.empty())
  {
    // 从任务队列中取出任务
    task = queue_.front();
    queue_.pop_front();// 任务队列pop()
    if (maxQueueSize_ > 0)// ???
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      // 获取任务
      // runInThread()函数调用take()函数 take()返回任务队列中的任务函数--一个函数指针/地址
      Task task(take());// take()处于阻塞状态 等待任务队列不为空
      if (task)
      {
        task();// 执行任务task--执行函数指针对应的函数
      }
    }
  }
  catch (const Exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

