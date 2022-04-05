// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Types.h"

#include <deque>
#include <vector>

namespace muduo
{

class ThreadPool : noncopyable
{
 public:
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }

  // 启动线程池 numThreads：启动的初始线程个数
  // 完成线程池的初始化
  void start(int numThreads);
  // 关闭线程池
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // Call after stop() will return immediately.
  // There is no move-only version of std::function in C++ as of C++14.
  // So we don't need to overload a const& and an && versions
  // as we do in (Bounded)BlockingQueue.
  // https://stackoverflow.com/a/25408989
  // 向任务队列中添加任务Task f 生产者
  void run(Task f);

 private:
  bool isFull() const REQUIRES(mutex_);
  void runInThread();// 调用take()函数实现从线程池中取任务
  Task take();// 去线程池中取任务 消费者

  mutable MutexLock mutex_;
  Condition notEmpty_ GUARDED_BY(mutex_);
  Condition notFull_ GUARDED_BY(mutex_);
  string name_;// 线程池的名称
  Task threadInitCallback_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_;// 线程队列 消费者
  std::deque<Task> queue_ GUARDED_BY(mutex_);// 任务队列 存储任务的队列
  size_t maxQueueSize_;
  bool running_;// 标志线程池是否处于运行状态
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADPOOL_H
