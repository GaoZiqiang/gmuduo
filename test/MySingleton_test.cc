#include "muduo/base/Singleton.h"
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

class TestNoDestroy : muduo::noncopyable
{
 public:
  // Tag member for Singleton<T>
  void no_destroy();

  TestNoDestroy()
  {
    printf("tid=%d, constructing TestNoDestroy %p\n", muduo::CurrentThread::tid(), this);
  }

  ~TestNoDestroy()
  {
    printf("tid=%d, destructing TestNoDestroy %p\n", muduo::CurrentThread::tid(), this);
  }
};

void threadFunc()
{
  printf("tid=%d, %p name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::Singleton<Test>::instance(),
         muduo::Singleton<Test>::instance().name().c_str());
  // 修改name
  muduo::Singleton<Test>::instance().setName("only one, changed");
}

int main()
{
  // muduo::Singleton<Test>::instance() 返回Test类的单例对象
  // 先创建单例对象 set name
  muduo::Singleton<Test>::instance().setName("only one");
  // 创建新线程 修改name
  // 子线程与主线程共享同一个Test单例 子线程共享主线程的资源
  muduo::Thread t1(threadFunc);
  t1.start();
  t1.join();
  // 输出修改后的name
  printf("tid=%d, %p name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::Singleton<Test>::instance(),
         muduo::Singleton<Test>::instance().name().c_str());
  muduo::Singleton<TestNoDestroy>::instance();
  printf("with valgrind, you should see %zd-byte memory leak.\n", sizeof(TestNoDestroy));
}
