#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace muduo;

class Test
{
 public:
  Test(int numThreads)
    : latch_(1),
      threads_(numThreads)
  {
    // 创建三个线程
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.push_back(new muduo::Thread(
            boost::bind(&Test::threadFunc, this), muduo::string(name)));
    }
    // 启动各子线程 从begin到end 执行Thread::start函数
    for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));
  }

  // 发号施令
  void run()
  {
      // count_减1 减少至0时 就开始
    latch_.countDown();
  }

  void joinAll()
  {
    for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
  }

 private:

  void threadFunc()
  {
    // 等待主线程发号施令
    latch_.wait();
    printf("tid=%d, %s started\n",
           CurrentThread::tid(),
           CurrentThread::name());

    printf("tid=%d, %s stopped\n",
           CurrentThread::tid(),
           CurrentThread::name());
  }

  CountDownLatch latch_;
  boost::ptr_vector<Thread> threads_;
};

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
  Test t(3);
  sleep(3);
  printf("pid=%d, tid=%d %s running ...\n", ::getpid(), CurrentThread::tid(), CurrentThread::name());
  // 发号施令 主线程发号施令后各子线程才能执行
  t.run();
  t.joinAll();

  // 打印已启动的线程个数
  printf("number of created threads %d\n", Thread::numCreated());
}


