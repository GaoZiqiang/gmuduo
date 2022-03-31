// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
  : mutex_(),// 创造一个mutex_对象
    condition_(mutex_),// 将mutex_对象传到condition_里面
    count_(count)
{
}

void CountDownLatch::wait()
{
  MutexLockGuard lock(mutex_);// 保护count_值 可能多个线程都会访问count_变量
  while (count_ > 0)
  {
    condition_.wait();
  }
  // 离开CountDownLatch::wait()函数的作用域后，lock会自动解锁
}

void CountDownLatch::countDown()
{
  MutexLockGuard lock(mutex_);// 保护count值
  --count_;// count减1
  if (count_ == 0)
  {
    condition_.notifyAll();// 通知所有的等待线程
  }
}

int CountDownLatch::getCount() const
{
    // mutex_为mutable变量 可以修改
  MutexLockGuard lock(mutex_);// 保护count值
  return count_;// 返回count值
}

