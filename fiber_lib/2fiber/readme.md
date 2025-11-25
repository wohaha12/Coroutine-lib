# 协程库 (Coroutine Library)

## 项目简介

这是一个基于C++实现的轻量级用户级协程库，使用`ucontext_t`机制实现协程的创建、切换和调度。该库提供了简洁而强大的协程API，允许开发者轻松实现协作式多任务，适合高并发、IO密集型应用场景。

## 为什么要实现协程库

### 协程的优势

1. **轻量级**：相比线程，协程的创建和切换开销极小，内存占用也更小
2. **高并发**：单个线程可以运行成千上万个协程，大幅提高并发处理能力
3. **协作式调度**：协程主动让出CPU，避免了线程切换的上下文开销
4. **简化异步编程**：使用协程可以以同步的方式编写异步代码，提高代码可读性和可维护性
5. **IO密集型任务优化**：特别适合网络IO、文件IO等密集型操作，可以充分利用CPU资源

### 应用场景

- 网络服务器（如Web服务器、代理服务器）
- 爬虫系统
- 数据处理管道
- 需要处理大量并发连接的应用

## 实现原理

### 核心技术

本协程库基于Linux/Unix系统的`ucontext_t`机制实现，主要使用以下系统函数：

- `getcontext()`: 获取当前执行上下文
- `makecontext()`: 创建一个新的执行上下文
- `swapcontext()`: 切换执行上下文

### 设计思路

1. **协程状态管理**：
   - READY: 协程已创建但尚未运行或已让出执行权
   - RUNNING: 协程正在执行中
   - TERM: 协程执行完毕

2. **协程栈管理**：
   - 为每个协程分配独立的栈空间（默认128KB）
   - 主协程使用线程默认栈

3. **线程局部存储**：
   - 使用`thread_local`存储当前线程的协程信息
   - 跟踪正在运行的协程、主协程和调度协程

4. **生命周期管理**：
   - 使用智能指针（`std::shared_ptr`）管理协程生命周期
   - 避免内存泄漏和悬挂指针

### 主要组件

1. **Fiber类**：核心协程类，提供协程的创建、切换、恢复和销毁功能
2. **协程调度**：支持简单的协程调度器实现（如test.cpp中的Scheduler类）
3. **上下文管理**：负责协程执行上下文的保存和恢复

## 如何使用

### 编译

```bash
g++ *.cpp -std=c++17 -o test
```

### 基本使用流程

1. **初始化主协程**
   在使用协程前，需要先初始化当前线程的主协程：
   ```cpp
   Fiber::GetThis();
   ```

2. **创建子协程**
   ```cpp
   std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(
       []() {
           // 协程要执行的任务
           std::cout << "Hello from fiber!" << std::endl;
       },
       128000,  // 栈大小（可选）
       false    // 是否在调度器中运行（可选）
   );
   ```

3. **恢复协程执行**
   ```cpp
   fiber->resume();
   ```

4. **协程让出执行权**
   协程内部可以主动让出执行权：
   ```cpp
   Fiber::GetThis()->yield();
   ```

5. **重置已完成的协程**
   当协程执行完毕后，可以重置它以复用：
   ```cpp
   if (fiber->getState() == Fiber::TERM) {
       fiber->reset([]() {
           std::cout << "Fiber reused!" << std::endl;
       });
   }
   ```

### 示例代码

参考`test.cpp`中的实现，以下是一个简单的协程使用示例：

```cpp
#include "fiber.h"

void test_task() {
    std::cout << "Task running in fiber " << Fiber::GetFiberId() << std::endl;
    // 模拟工作
    Fiber::GetThis()->yield();  // 主动让出执行权
    std::cout << "Task resumed in fiber " << Fiber::GetFiberId() << std::endl;
}

int main() {
    // 初始化主协程
    Fiber::GetThis();
    
    std::cout << "Main fiber started" << std::endl;
    
    // 创建子协程
    std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(test_task, 0, false);
    
    // 恢复协程执行
    fiber->resume();
    
    std::cout << "Main fiber resumed" << std::endl;
    
    // 再次恢复协程执行
    fiber->resume();
    
    std::cout << "Main fiber finished" << std::endl;
    
    return 0;
}
```

### 调试信息

可以在`fiber.cpp`中通过修改以下变量来开启或关闭调试信息：

```cpp
static bool debug = true;  // true表示开启调试信息，false表示关闭
```

开启调试后，将输出协程的创建、销毁和状态变化信息。

## 注意事项

1. **栈溢出**：注意协程栈大小限制，避免在协程中使用过大的局部变量
2. **异常处理**：协程中的异常需要在协程内部捕获，否则可能导致程序崩溃
3. **资源管理**：确保在协程中正确管理资源，避免资源泄漏
4. **线程安全**：协程是线程局部的，不同线程的协程不能直接切换
5. **阻塞操作**：避免在协程中执行阻塞操作，这会阻塞整个线程中的所有协程

## 扩展与优化

本协程库提供了基础功能，可以根据需要进行扩展：

1. 实现更复杂的调度器，支持任务优先级和超时控制
2. 添加协程池，减少协程创建和销毁的开销
3. 支持协程间通信机制，如channel或消息队列
4. 集成事件驱动框架，实现高效的异步IO
