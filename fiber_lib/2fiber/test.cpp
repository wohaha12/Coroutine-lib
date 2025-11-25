#include "fiber.h"
#include <vector>

// 使用sylar命名空间
using namespace sylar; 

/**
 * @brief 简单的协程调度器类
 * @details 该调度器负责管理协程任务队列，并按顺序执行这些任务
 *          这是一个基础的调度器实现，仅用于演示协程的基本使用
 */
class Scheduler
{
public:
    /**
     * @brief 添加协程调度任务
     * @param task 要调度的协程智能指针
     * @details 将协程任务添加到任务队列中，等待执行
     */
    void schedule(std::shared_ptr<Fiber> task)
    {
        m_tasks.push_back(task);
    }

    /**
     * @brief 执行调度任务
     * @details 按照添加顺序遍历任务队列，依次执行每个协程任务
     *          每个协程执行完毕后会自动切换回主协程，继续执行下一个任务
     */
    void run()
    {
        std::cout << " number " << m_tasks.size() << std::endl;

        std::shared_ptr<Fiber> task;
        auto it = m_tasks.begin();
        
        // 遍历任务队列中的所有协程任务
        while(it != m_tasks.end())
        {
            // 获取当前任务
            task = *it;
            
            // 恢复协程执行，从主协程切换到子协程
            // 子协程函数运行完毕后会自动切换回主协程
            task->resume();
            
            // 移动到下一个任务
            it++;
        }
        
        // 清空任务队列
        m_tasks.clear();
    }

private:
    // 任务队列，存储待执行的协程任务
    std::vector<std::shared_ptr<Fiber>> m_tasks;
};

/**
 * @brief 测试协程函数
 * @param i 协程编号，用于标识不同的协程
 * @details 这是一个简单的协程任务函数，仅输出一条包含编号的消息
 */
void test_fiber(int i)
{
    std::cout << "hello world " << i << std::endl;
}

int main()
{
    // 初始化当前线程的主协程
    // 这是使用协程库的第一步，必须先调用此函数
    Fiber::GetThis();

    // 创建协程调度器实例
    Scheduler sc;

    // 添加多个调度任务（任务和子协程绑定）
    for(auto i = 0; i < 20; i++)
    {
        // 创建子协程
        // 1. 使用共享指针(std::shared_ptr)自动管理协程资源
        // 2. 使用std::bind绑定函数和参数，生成一个可调用对象
        // 3. 每个协程执行test_fiber函数，传入不同的参数i
        // 4. stacksize设为0，表示使用默认栈大小
        // 5. run_in_scheduler设为false，表示不在调度器上下文中运行
        std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(std::bind(test_fiber, i), 0, false);
        
        // 将协程任务添加到调度器
        sc.schedule(fiber);
    }

    // 执行调度任务
    // 调度器会依次执行所有添加的协程任务
    sc.run();

    return 0;
}