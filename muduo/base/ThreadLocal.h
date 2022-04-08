// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include "muduo/base/Mutex.h"
#include "muduo/base/noncopyable.h"

#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocal : noncopyable
{
 public:
  ThreadLocal()
  {
    // 创建key
    MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor));
  }

  ~ThreadLocal()
  {
    // 销毁key 并不是销毁实际的数据 数据在堆区域中
    MCHECK(pthread_key_delete(pkey_));
  }

  T& value()
  {
    // 转换为T*的指针
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));// pthread_getspecific获取线程特定数据
    // 若返回为空 则表明该数据还未创建-->创建该数据 为该pkey_创建值 并赋值 pkey_存储的是值的地址/指针
    if (!perThreadValue)
    {
      // 构造对象
      T* newObj = new T();
      // 并将值的地址T*赋值为pkey_
      MCHECK(pthread_setspecific(pkey_, newObj));
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }

 private:

  // pthread_key_create()函数传入的回调函数 实现对实际数据的删除
  static void destructor(void *x)
  {
    T* obj = static_cast<T*>(x);
    // 检测是否为完全类型
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;// ？？？
    // 调用delete销毁 删除堆空间中的数据
    delete obj;
  }

 private:
  pthread_key_t pkey_;
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADLOCAL_H
