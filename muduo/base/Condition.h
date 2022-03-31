// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "muduo/base/Mutex.h"

#include <pthread.h>

namespace muduo
{

class Condition : noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)// 不拥有 也不负责管理mutex变量的生存期
    : mutex_(mutex)
  {
    // 创建条件变量pcond_
    MCHECK(pthread_cond_init(&pcond_, NULL));
  }

  ~Condition()
  {
    // 销毁条件变量
    MCHECK(pthread_cond_destroy(&pcond_));
  }

  // wait函数
  void wait()
  {
    MutexLock::UnassignGuard ug(mutex_);
    // 调用pthread_cond_wait()函数
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
  }

  // returns true if time out, false otherwise.
  bool waitForSeconds(double seconds);

  // 即signal()函数
  void notify()
  {
    // 调用pthread_cond_signal()函数
    MCHECK(pthread_cond_signal(&pcond_));
  }

  void notifyAll()
  {
    MCHECK(pthread_cond_broadcast(&pcond_));
  }

 private:
  MutexLock& mutex_;// MutexLock mutexLock
  pthread_cond_t pcond_;// 条件变量
};

}  // namespace muduo

#endif  // MUDUO_BASE_CONDITION_H
