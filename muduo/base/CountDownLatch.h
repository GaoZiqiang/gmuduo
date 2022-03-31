// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

namespace muduo
{

class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  // ???
  void wait();

  // 计数器减一-->计数器减为0之后便向所有的线程发起通知
  void countDown();

  // 获取当前计数器的值
  int getCount() const;

 private:
  mutable MutexLock mutex_;// mutable:可以在const修饰该变量时，改变该变量的值 参考CountDownLatch::getCount()
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);// 计数器
};

}  // namespace muduo
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
