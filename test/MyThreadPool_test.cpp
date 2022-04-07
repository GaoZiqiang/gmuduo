#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"
#include "MyThreadPool.h"

#include <stdio.h>
#include <unistd.h>  // usleep

void print()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
    LOG_INFO << str;
    usleep(100*1000);
}

void test(int maxSize)
{
    LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
    muduo::ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    // 初始化线程池
    pool.start(5);

    // 向任务队列中添加2个任务
    LOG_WARN << "Adding";
    pool.run(print);
    pool.run(print);
    // 再添加100个任务
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(std::bind(printString, std::string(buf)));
    }
    LOG_WARN << "Done";

    // 门栓类？
    muduo::CountDownLatch latch(1);
    pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));// bind创建的latch对象(this指针)的所有成员变量
    // test()函数阻塞在这里了
    latch.wait();// 等待latch对象的count_减为0
    pool.stop();
}

int main() {
    test(5);
}