#include "muduo/base/ThreadLocal.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Thread.h"

#include <stdio.h>

class Test : muduo::noncopyable
{
 public:
  Test()
  {
    printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const muduo::string& name() const { return name_; }
  void setName(const muduo::string& n) { name_ = n; }

 private:
  muduo::string name_;
};

muduo::ThreadLocal<Test> testObj1;
muduo::ThreadLocal<Test> testObj2;

void print()
{
  printf("tid=%d, obj1 %p name=%s\n",
         muduo::CurrentThread::tid(),
         &testObj1.value(),
         testObj1.value().name().c_str());
  printf("tid=%d, obj2 %p name=%s\n",
         muduo::CurrentThread::tid(),
         &testObj2.value(),
         testObj2.value().name().c_str());
}

void threadFunc()
{
  // 此时新线程特有的两个Test对象还没有赋值/初始化
  print();
  /*
   * tid=18785, constructing 0x7fb7dc000b20
   * tid=18785, obj1 0x7fb7dc000b20 name=
   * tid=18785, constructing 0x7fb7dc000b50
   * tid=18785, obj2 0x7fb7dc000b50 name=
   * */
  // 新线程特有的两个Test对象进行赋值/初始化
  testObj1.value().setName("changed 1");
  testObj2.value().setName("changed 42");
  // 此时新线程特有的两个Test对象已经有了自己的特有值
  print();
  /*
   * tid=18785, obj1 0x7fb7dc000b20 name=changed 1
   * tid=18785, obj2 0x7fb7dc000b50 name=changed 42
   * */
}

int main()
{
  testObj1.value().setName("main one");
  /*
   * tid=18784, constructing 0x555bea1a3e70
   * */
  print();
  /*
   * tid=18784, obj1 0x555bea1a3e70 name=main one
   * tid=18784, constructing 0x555bea1a42b0
   * tid=18784, obj2 0x555bea1a42b0 name=
   * */
  muduo::Thread t1(threadFunc);
  t1.start();
  // 子线程执行完后 其所特有的两个Test对象被销毁
  t1.join();
  /*
   * tid=18785, destructing 0x7fb7dc000b20 changed 1
   * tid=18785, destructing 0x7fb7dc000b50 changed 42
   * */
  testObj2.value().setName("main two");
  print();
  /*
   * tid=18784, obj1 0x555bea1a3e70 name=main one
   * tid=18784, obj2 0x555bea1a42b0 name=main two
   * */

  // 主线程执行完后 其所特有的两个Test对象被销毁
  pthread_exit(0);
  /*
   * tid=18784, destructing 0x555bea1a3e70 main one
   * tid=18784, destructing 0x555bea1a42b0 main two
   * */
}
