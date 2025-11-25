#include "thread.h"               // 包含线程库头文件
#include <iostream>                // 用于输出信息
#include <memory>                  // 用于使用智能指针
#include <vector>                  // 用于使用vector容器
#include <unistd.h>                // 用于sleep函数

// 使用sylar命名空间
using namespace sylar;

/**
 * @brief 线程执行函数
 * 
 * 该函数将在每个创建的线程中执行，用于演示如何获取线程信息
 */
void func()
{
    // 输出线程信息：
    // 1. 使用静态方法GetThreadId()和GetName()获取当前线程的ID和名称
    // 2. 使用GetThis()获取当前线程对象，然后调用getId()和getName()获取信息
    std::cout << "id: " << Thread::GetThreadId() << ", name: " << Thread::GetName();
    std::cout << ", this id: " << Thread::GetThis()->getId() << ", this name: " << Thread::GetThis()->getName() << std::endl;

    // 使线程休眠60秒，模拟线程执行耗时操作
    sleep(60);
}

/**
 * @brief 主函数
 * 
 * 演示如何使用Thread类创建和管理多个线程
 */
int main() {
    // 创建一个vector容器，用于存储线程对象的智能指针
    std::vector<std::shared_ptr<Thread>> thrs;

    // 创建5个线程
    for(int i=0; i<5; i++)
    {
        // 使用智能指针创建线程对象
        // 第一个参数是线程要执行的函数
        // 第二个参数是线程名称，格式为"thread_"+序号
        std::shared_ptr<Thread> thr = std::make_shared<Thread>(&func, "thread_"+std::to_string(i));
        // 将线程对象添加到vector中
        thrs.push_back(thr);
    }

    // 等待所有线程执行完成
    for(int i=0; i<5; i++)
    {
        // 调用join()方法，阻塞当前线程直到thrs[i]线程执行完毕
        thrs[i]->join();
    }

    return 0;
}