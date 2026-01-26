# 协程调度器模块 (Scheduler)

## 1. 模块概述

协程调度器是 mycoroutine 库的核心组件之一，负责管理线程池和协程任务的调度。该模块实现了高效的任务调度机制，支持将任务分配到不同的线程执行，充分利用多核 CPU 资源，提高程序的并发处理能力。

### 1.1 主要功能
- 线程池管理：创建和管理工作线程
- 任务队列：线程安全的任务存储和分配
- 协程调度：将协程任务分配到合适的线程执行
- 线程唤醒机制：当有新任务时唤醒空闲线程
- 空闲线程管理：处理无任务时的线程状态
- 支持指定任务执行线程：可以将任务分配到特定线程

### 1.2 设计目标
- 高效调度：低延迟的任务分配和执行
- 负载均衡：任务均匀分配到各个工作线程
- 可扩展性：支持动态调整线程池大小
- 易用性：提供简洁的 API 接口
- 稳定性：完善的错误处理和资源管理

## 2. 核心设计

### 2.1 调度器结构设计

```cpp
class Scheduler {
public:
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name="Scheduler");
    virtual ~Scheduler();
    
    const std::string& getName() const;
    
    static Scheduler* GetThis();
    
protected:
    void SetThis();
    
public:
    template <class FiberOrCb>
    void scheduleLock(FiberOrCb fc, int thread = -1);
    
    virtual void start();
    virtual void stop();    
    
protected:
    virtual void tickle();
    virtual void run();
    virtual void idle();
    virtual bool stopping();
    
    bool hasIdleThreads();
    
private:
    struct ScheduleTask {
        std::shared_ptr<Fiber> fiber;  // 协程指针
        std::function<void()> cb;      // 回调函数
        int thread;                    // 指定任务需要运行的线程id
        
        // 构造函数和重置方法
    };
    
private:
    std::string m_name;                  // 调度器名称
    bool m_useCaller;                    // 主线程是否用作工作线程
    std::mutex m_mutex;                  // 互斥锁，保护任务队列
    std::vector<std::shared_ptr<Thread>> m_threads;  // 线程池
    std::vector<ScheduleTask> m_tasks;   // 任务队列
    std::vector<int> m_threadIds;        // 工作线程的线程ID列表
    size_t m_threadCount = 0;            // 需要额外创建的线程数
    std::atomic<size_t> m_activeThreadCount = {0};  // 活跃线程数
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数
    std::shared_ptr<Fiber> m_schedulerFiber;  // 调度协程
    int m_rootThread = -1;               // 主线程ID
    bool m_stopping = false;             // 是否正在关闭调度器
};
```

### 2.2 任务结构

调度器使用 `ScheduleTask` 结构体来表示一个任务，支持两种类型的任务：

- **协程任务**：通过 `fiber` 指针指定要执行的协程
- **回调任务**：通过 `cb` 函数对象指定要执行的回调函数

每个任务还可以指定执行线程，通过 `thread` 字段控制：
- `thread = -1`：任务可以在任意线程执行
- `thread = N`：任务必须在指定 ID 为 N 的线程执行

### 2.3 线程池管理

调度器支持两种线程使用模式：

1. **使用调用者线程**：当 `use_caller = true` 时，调用者线程也会作为工作线程参与任务执行
2. **不使用调用者线程**：当 `use_caller = false` 时，仅使用新创建的线程作为工作线程

线程池的大小由构造函数的 `threads` 参数决定，表示需要额外创建的线程数。实际工作线程数为：
- 如果 `use_caller = true`：实际线程数 = `threads + 1`
- 如果 `use_caller = false`：实际线程数 = `threads`

### 2.4 任务调度流程

```
1. 用户调用 scheduleLock() 添加任务
2. 任务被添加到线程安全的任务队列
3. 如果队列之前为空，调用 tickle() 唤醒空闲线程
4. 工作线程从任务队列获取任务
5. 如果任务指定了线程，检查当前线程是否匹配
6. 如果匹配，执行任务；否则，将任务放回队列
7. 执行任务：
   - 如果是协程任务，恢复协程执行
   - 如果是回调任务，创建临时协程执行回调
8. 任务执行完毕后，继续从队列获取下一个任务
9. 如果队列中没有任务，执行 idle() 函数
```

## 3. API 接口说明

### 3.1 构造与析构

#### 3.1.1 Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name="Scheduler")

**功能**：创建协程调度器

**参数**：
- `threads`：线程池大小（额外创建的线程数），默认为 1
- `use_caller`：是否将调用者线程作为工作线程，默认为 true
- `name`：调度器名称，默认为 "Scheduler"

**返回值**：无

**使用示例**：
```cpp
// 创建一个包含2个额外线程的调度器，使用调用者线程
auto scheduler = std::make_shared<mycoroutine::Scheduler>(2, true, "MyScheduler");
```

#### 3.1.2 ~Scheduler()

**功能**：调度器析构函数，释放资源

**参数**：无

**返回值**：无

### 3.2 任务调度

#### 3.2.1 template <class FiberOrCb> void scheduleLock(FiberOrCb fc, int thread = -1)

**功能**：添加任务到任务队列（线程安全）

**参数**：
- `fc`：任务对象，可以是协程指针或回调函数
- `thread`：指定任务执行的线程 ID，默认为 -1（任意线程）

**返回值**：无

**说明**：该函数是一个模板函数，支持多种类型的任务参数

**使用示例**：
```cpp
// 添加协程任务
auto fiber = std::make_shared<mycoroutine::Fiber>([](){
    std::cout << "Fiber task" << std::endl;
});
scheduler->scheduleLock(fiber);

// 添加回调任务
scheduler->scheduleLock([](){
    std::cout << "Callback task" << std::endl;
});

// 指定线程执行任务
scheduler->scheduleLock([](){
    std::cout << "Task on specific thread" << std::endl;
}, 1);  // 在ID为1的线程执行
```

### 3.3 调度器控制

#### 3.3.1 virtual void start()

**功能**：启动线程池

**参数**：无

**返回值**：无

**说明**：创建并启动所有工作线程

**使用示例**：
```cpp
// 启动调度器
scheduler->start();
```

#### 3.3.2 virtual void stop()

**功能**：关闭线程池

**参数**：无

**返回值**：无

**说明**：等待所有任务完成，并停止所有工作线程

**使用示例**：
```cpp
// 关闭调度器
scheduler->stop();
```

### 3.4 状态查询

#### 3.4.1 const std::string& getName() const

**功能**：获取调度器名称

**参数**：无

**返回值**：调度器名称的常量引用

#### 3.4.2 static Scheduler* GetThis()

**功能**：获取正在运行的调度器

**参数**：无

**返回值**：当前线程的调度器指针

#### 3.4.3 bool hasIdleThreads()

**功能**：检查是否有空闲线程

**参数**：无

**返回值**：是否有空闲线程

### 3.5 保护成员函数

#### 3.5.1 void SetThis()

**功能**：设置正在运行的调度器

**参数**：无

**返回值**：无

**说明**：将当前调度器实例设置为线程局部存储的调度器

#### 3.5.2 virtual void tickle()

**功能**：唤醒线程函数

**参数**：无

**返回值**：无

**说明**：通知其他线程有新任务到来，默认实现为空，由子类重写

#### 3.5.3 virtual void run()

**功能**：工作线程主函数

**参数**：无

**返回值**：无

**说明**：从任务队列获取任务并执行

#### 3.5.4 virtual void idle()

**功能**：空闲协程函数

**参数**：无

**返回值**：无

**说明**：当没有任务时执行，默认实现为无限循环

#### 3.5.5 virtual bool stopping()

**功能**：判断调度器是否可以停止

**参数**：无

**返回值**：调度器是否可以停止的标志

**说明**：默认实现检查任务队列是否为空和活跃线程数

## 4. 实现原理

### 4.1 线程池创建

当调用 `start()` 方法时，调度器会创建指定数量的工作线程：

1. 如果 `use_caller = true`，则将调用者线程作为一个工作线程
2. 创建 `threads` 数量的额外工作线程
3. 每个工作线程执行 `run()` 方法
4. 将所有工作线程的 ID 存储到 `m_threadIds` 列表

### 4.2 任务调度机制

调度器使用线程安全的任务队列来存储待执行的任务。当有新任务添加到队列时，如果队列之前为空，会调用 `tickle()` 方法唤醒空闲线程。

工作线程的 `run()` 方法执行以下逻辑：

1. 设置当前线程的调度器为 this
2. 获取当前线程的主协程
3. 进入循环：
   a. 加锁获取任务队列中的任务
   b. 如果没有任务，执行 `idle()` 函数
   c. 如果有任务，遍历任务队列
   d. 找到适合当前线程执行的任务
   e. 从队列中移除任务
   f. 解锁队列
   g. 执行任务
   h. 重复步骤 a

### 4.3 任务执行

任务执行分为两种情况：

1. **协程任务**：直接恢复协程执行，协程执行完毕后自动让出
2. **回调任务**：创建临时协程，将回调函数作为协程入口，执行完毕后销毁协程

### 4.4 空闲线程处理

当没有任务可执行时，工作线程会执行 `idle()` 函数。默认的 `idle()` 实现是一个无限循环，不断检查是否有新任务。子类可以重写 `idle()` 函数，实现自定义的空闲处理逻辑，例如休眠一段时间后再次检查任务队列。

### 4.5 调度器停止机制

当调用 `stop()` 方法时，调度器会：

1. 设置 `m_stopping` 标志为 true
2. 唤醒所有工作线程
3. 等待所有工作线程退出
4. 释放资源

工作线程在执行 `run()` 方法时，会定期检查 `stopping()` 方法的返回值。当 `stopping()` 返回 true 时，工作线程会退出循环，结束执行。

## 5. 使用示例

### 5.1 简单调度器使用

```cpp
#include <iostream>
#include <mycoroutine/scheduler.h>
#include <mycoroutine/fiber.h>

int main() {
    // 创建调度器，使用2个额外线程，使用调用者线程
    mycoroutine::Scheduler scheduler(2, true, "TestScheduler");
    
    // 启动调度器
    scheduler.start();
    
    // 添加10个任务
    for (int i = 0; i < 10; ++i) {
        scheduler.scheduleLock([i](){
            std::cout << "Task " << i << " running in thread " << mycoroutine::GetThreadId() << std::endl;
            // 模拟任务执行
            usleep(10000);
        });
    }
    
    // 关闭调度器
    scheduler.stop();
    
    std::cout << "All tasks completed" << std::endl;
    
    return 0;
}
```

### 5.2 指定任务执行线程

```cpp
#include <iostream>
#include <mycoroutine/scheduler.h>

int main() {
    // 创建调度器，使用3个额外线程
    mycoroutine::Scheduler scheduler(3, true, "TestScheduler");
    scheduler.start();
    
    // 获取工作线程ID列表
    std::vector<int> thread_ids = {0, 1, 2, 3};  // 假设调用者线程ID为0
    
    // 添加任务到指定线程
    for (int i = 0; i < 4; ++i) {
        scheduler.scheduleLock([i, thread_ids](){
            std::cout << "Task " << i << " running in thread " << mycoroutine::GetThreadId() 
                      << " (expected: " << thread_ids[i] << ")" << std::endl;
        }, thread_ids[i]);
    }
    
    scheduler.stop();
    
    return 0;
}
```

### 5.3 结合协程使用

```cpp
#include <iostream>
#include <mycoroutine/scheduler.h>
#include <mycoroutine/fiber.h>

int main() {
    mycoroutine::Scheduler scheduler(2, true, "CoroutineScheduler");
    scheduler.start();
    
    // 创建协程并添加到调度器
    for (int i = 0; i < 5; ++i) {
        auto fiber = std::make_shared<mycoroutine::Fiber>([i](){
            std::cout << "Fiber " << i << " start in thread " << mycoroutine::GetThreadId() << std::endl;
            
            // 让出执行权
            mycoroutine::Fiber::GetThis()->yield();
            
            std::cout << "Fiber " << i << " resume in thread " << mycoroutine::GetThreadId() << std::endl;
        });
        
        scheduler.scheduleLock(fiber);
    }
    
    scheduler.stop();
    
    return 0;
}
```

## 6. 性能优化

### 6.1 任务队列优化

- 使用 `std::vector` 作为任务队列，避免频繁的内存分配
- 任务添加和获取都使用锁保护，但锁的持有时间短
- 当队列中有任务时，工作线程不会频繁加锁，而是批量处理任务

### 6.2 线程唤醒机制

- 当任务队列从空变为非空时，才唤醒空闲线程
- 避免了不必要的线程唤醒，减少了系统开销
- 子类可以重写 `tickle()` 方法，实现更高效的唤醒机制（如使用 eventfd）

### 6.3 负载均衡

- 任务均匀分配到各个工作线程
- 支持指定任务执行线程，可以实现负载倾斜
- 活跃线程数和空闲线程数的统计，便于监控系统状态

## 7. 注意事项

### 7.1 线程安全

- 任务队列是线程安全的，可以从任意线程添加任务
- 调度器的其他方法不是线程安全的，应避免从多个线程同时调用
- 每个线程只能有一个活跃的调度器

### 7.2 任务依赖

- 调度器不处理任务之间的依赖关系
- 如果任务之间有依赖，需要用户自行处理
- 可以使用同步原语（如互斥锁、条件变量）来实现任务同步

### 7.3 避免死锁

- 避免在任务中长时间持有锁
- 避免任务之间的循环依赖
- 避免在 `idle()` 函数中执行阻塞操作

### 7.4 内存管理

- 调度器使用智能指针管理线程和协程，避免资源泄漏
- 回调任务执行时会创建临时协程，执行完毕后自动销毁
- 用户需要确保任务中使用的资源在任务执行期间有效

## 8. 总结

协程调度器模块是 mycoroutine 库的核心组件，实现了高效的任务调度机制。该模块支持线程池管理、任务队列、协程调度、线程唤醒机制等功能，可以充分利用多核 CPU 资源，提高程序的并发处理能力。

调度器的设计具有以下特点：

- 高效调度：低延迟的任务分配和执行
- 负载均衡：任务均匀分配到各个工作线程
- 可扩展性：支持动态调整线程池大小
- 易用性：提供简洁的 API 接口
- 稳定性：完善的错误处理和资源管理

该模块为上层的 IO 管理器等组件提供了基础支持，是整个协程库的重要组成部分。