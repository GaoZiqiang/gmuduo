// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include "muduo/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocalSingleton : noncopyable
{
 public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;

  static T& instance()
  {
    // 每个线程都有一个t_value__指针
    // 因为t_value__指针每个线程都有一个，因此不需要像Singleton那样进行加锁
    if (!t_value_)
    {
      t_value_ = new T();
      // 为
      deleter_.set(t_value_);
    }
    return *t_value_;// 返回对象引用
  }

  static T* pointer()
  {
    return t_value_;// 返回指针
  }

 private:
  static void destructor(void* obj)
  {
    assert(obj == t_value_);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete t_value_;
    t_value_ = 0;
  }

  // 嵌套类 自动销毁t_value_指针
  class Deleter
  {
   public:
    Deleter()
    {
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);// 指定回调函数为ThreadLocalSingleton::destructor
    }

    ~Deleter()
    {
      // 销毁数据
      pthread_key_delete(pkey_);// 通过指定的ThreadLocalSingleton::destructor回调函数销毁pkey_指针
    }

    void set(T* newObj)
    {
      // 通过TSD Thread-specific Data实现
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;
  };

  static __thread T* t_value_;// __thread关键字：表示t_value_指针每个线程都有一份
  static Deleter deleter_;// 用于销毁指针所指向的对象
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}  // namespace muduo
#endif  // MUDUO_BASE_THREADLOCALSINGLETON_H
