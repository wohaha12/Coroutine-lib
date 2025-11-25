#include <iostream>
#include <mutex>
#include "scheduler.h"

using namespace sylar;

// 任务计数变量
static unsigned int test_number;
// 用于保护cout输出的互斥锁
std::mutex mutex_cout;

/**
 * @brief 原始测试任务函数
 * 输出任务编号和执行线程ID，并休眠1秒
 */
void task()
{
    {
        std::lock_guard<std::mutex> lock(mutex_cout);
        std::cout << "task " << test_number ++ << " is under processing in thread: " << Thread::GetThreadId() << std::endl;        
    }
    sleep(1);
}

// 互斥锁，用于保护cout输出，避免多线程输出混乱
std::mutex g_mutex;

/**
 * @brief 测试任务函数，模拟一些工作并输出任务信息
 * @param i 任务ID，用于标识不同的任务
 */
void test_fiber(int i)
{
    {   
        // 使用互斥锁保护输出，确保输出不被打断
        std::lock_guard<std::mutex> lock(g_mutex);
        std::cout << "Hello world " << i << " tid=" << Thread::GetThreadId() << std::endl;
    }
    
    // 协程让出执行权，切换回调度器
    Fiber::GetThis()->yield();
    
    {   
        std::lock_guard<std::mutex> lock(g_mutex);
        std::cout << "Hello world again " << i << " tid=" << Thread::GetThreadId() << std::endl;
    }
}

int main(int argc, char const *argv[])
{
    std::cout << "main begin" << " tid=" << Thread::GetThreadId() << std::endl;
    
    // 创建调度器对象
    // 参数1: 3 - 创建3个工作线程
    // 参数2: true - 使用调用者线程作为工作线程
    // 参数3: "sylar" - 调度器名称
    sylar::Scheduler sc(3, true, "sylar");
    
    // 启动调度器，开始运行工作线程
    sc.start();
    
    // 使用互斥锁保护输出
    {   
        std::lock_guard<std::mutex> lock(g_mutex);
        std::cout << "Schedule start" << std::endl;
    }
    
    // 向调度器添加5个任务
    for (int i = 0; i < 5; ++i)
    {
        // 任务函数是test_fiber，参数是i，第二个参数-1表示不指定特定线程
        sc.scheduleLock(std::bind(test_fiber, i), -1);
    }
    
    // 再添加15个任务，总共20个任务
    for (int i = 5; i < 20; ++i)
    {
        sc.scheduleLock(std::bind(test_fiber, i), -1);
    }
    
    // 停止调度器
    sc.stop();
    
    // 使用互斥锁保护输出
    {   
        std::lock_guard<std::mutex> lock(g_mutex);
        std::cout << "Schedule end" << std::endl;
    }
    
    std::cout << "main end" << " tid=" << Thread::GetThreadId() << std::endl;
    
    return 0;
}