// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include "muduo/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template<typename T>
struct has_no_destroy
{
  template <typename C> static char test(decltype(&C::no_destroy));
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace detail

// 模板类
template<typename T>
class Singleton : noncopyable
{
 public:
  // 私有成员函数
  Singleton() = delete;// delete声明该函数为私有函数 =default表示保留
  ~Singleton() = delete;

  // 该类唯一的公有成员
  static T& instance()
  {
    pthread_once(&ponce_, &Singleton::init);// pthread_once()保证init()函数只被调用一次 还能保证线程安全
    assert(value_ != NULL);
    return *value_;
  }

 private:
  static void init()
  {
    value_ = new T();
    // 程序结束时自动销毁该对象
    if (!detail::has_no_destroy<T>::value)
    {
      // atexit()注册销毁函数destroy()
      ::atexit(destroy);
    }
  }

  static void destroy()
  {
    // complete type
    // class A; // 只定义未声明-->非complete type
    // A* p; ... delete p; // 编译没有问题，但执行时会出问题
    // 若为非complete 则[]中为-1 数组不能为-1-->编译时报错 a[-1]
    // 保证对于非complete type尽量在编译阶段发现错误
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;

    // 销毁对象
    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_;// 保证一个函数只被执行一次 还能保证线程安全
  static T*             value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}  // namespace muduo

#endif  // MUDUO_BASE_SINGLETON_H
