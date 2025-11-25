# 线程库 (Thread Library)

## 1. 什么是线程库

这是一个基于POSIX线程库（pthread）的C++封装库，提供了更简洁、易用的线程管理接口。该库主要包含两个核心类：

- **Semaphore类**：实现了经典的信号量同步机制，用于线程间的同步操作
- **Thread类**：封装了pthread线程的创建、管理和控制功能

## 2. 为什么要实现这个线程库

实现这个线程库的主要目的包括：

- **简化线程操作**：提供更符合C++风格的接口，避免直接使用pthread API的复杂性
- **自动资源管理**：通过RAII原则确保线程资源的正确释放，避免资源泄漏
- **线程局部存储**：支持获取当前线程信息（ID、名称、线程对象），方便调试和日志记录
- **异常处理**：在关键操作失败时提供异常处理机制，使错误更加明确
- **类型安全**：使用std::function作为线程函数类型，支持各种可调用对象（函数、lambda、函数对象等）

## 3. 如何实现的

### 3.1 Semaphore类实现

Semaphore类基于C++标准库的互斥锁（std::mutex）和条件变量（std::condition_variable）实现：

- 使用互斥锁保护信号量计数器
- 使用条件变量实现线程等待和唤醒
- 实现了经典的P操作（wait）和V操作（signal）
- 使用while循环检查条件，避免虚假唤醒

### 3.2 Thread类实现

Thread类封装了pthread API，主要实现细节：

- **线程创建**：使用pthread_create创建线程，run静态方法作为入口点
- **线程局部存储**：使用thread_local存储当前线程对象和名称，方便全局访问
- **线程同步**：使用Semaphore确保线程完全初始化后才返回构造函数
- **智能指针优化**：使用swap操作减少回调函数中可能的智能指针引用计数问题
- **线程ID获取**：使用syscall(SYS_gettid)获取系统级线程ID，而非仅进程内有效的pthread_t
- **异常处理**：在线程创建或join失败时抛出异常并提供详细错误信息
- **资源管理**：在析构函数中使用pthread_detach确保线程资源被正确释放

## 4. 如何使用

### 4.1 基本使用步骤

1. 包含头文件：`#include "thread.h"`
2. 使用命名空间：`using namespace sylar;` 或直接使用`sylar::Thread`
3. 创建线程对象，指定执行函数和线程名称
4. 调用join()方法等待线程完成

### 4.2 示例代码

```cpp
// 创建一个要在线程中执行的函数
void threadFunc() {
    // 获取并打印线程信息
    std::cout << "Thread ID: " << Thread::GetThreadId() << std::endl;
    std::cout << "Thread Name: " << Thread::GetName() << std::endl;
    
    // 执行任务...
}

int main() {
    // 创建线程，指定执行函数和名称
    Thread thread(threadFunc, "my_thread");
    
    // 等待线程完成
    thread.join();
    
    return 0;
}
```

### 4.3 全局访问当前线程信息

在任何线程中，您都可以使用以下静态方法获取当前线程的信息：

- `Thread::GetThreadId()`: 获取当前线程的系统ID
- `Thread::GetName()`: 获取当前线程的名称
- `Thread::GetThis()`: 获取当前线程对象的指针
- `Thread::SetName(name)`: 设置当前线程的名称

### 4.4 智能指针管理线程

通常建议使用智能指针管理线程对象的生命周期：

```cpp
std::shared_ptr<Thread> thread = std::make_shared<Thread>(threadFunc, "my_thread");
thread->join();
```

## 5. 编译与运行

### 5.1 编译

g++ *.cpp -std=c++17 -o test

### 5.2 运行

./test

### 5.3 测试线程

1. 查看进程号 
ps uax | grep <name>
2. 查看该进程号下所有线程信息
ps -eLf | grep <pid>

## 6. 注意事项

- 线程名称限制为15个字符（系统限制）
- 确保在线程执行函数中正确处理异常，避免未捕获的异常导致程序终止
- 使用join()或确保线程在主线程结束前完成，避免孤儿线程
- 避免在析构函数中调用join()，可能导致死锁