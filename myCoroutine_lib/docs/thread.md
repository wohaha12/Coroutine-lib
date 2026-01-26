# 线程管理模块 (Thread)

## 1. 模块概述

线程管理模块是 mycoroutine 库的基础组件，负责封装线程相关的功能，提供易用的线程创建、管理和同步机制。该模块包含两个主要类：`Semaphore` 信号量类和 `Thread` 线程类，用于实现线程间的同步和线程的创建与管理。

### 1.1 主要功能
- 线程的创建、启动、等待和销毁
- 线程ID和名称的获取与设置
- 线程间的同步机制（信号量）
- 跨平台的线程操作封装

### 1.2 设计目标
- 易用性：提供简洁的API接口，隐藏底层实现细节
- 可靠性：确保线程的正确创建和销毁
- 安全性：提供线程安全的操作
- 可扩展性：支持自定义线程函数和线程属性

## 2. 核心设计

### 2.1 类结构设计

```cpp
// 信号量类，用于线程间同步
class Semaphore {
private:
    std::mutex mtx;                // 互斥锁，保护count的访问
    std::condition_variable cv;    // 条件变量，用于线程等待和唤醒
    int count;                     // 信号量计数器
    
public:
    explicit Semaphore(int count_ = 0);
    void wait();
    void signal();
};

// 线程类，封装pthread线程库
class Thread {
private:
    pid_t m_id;                    // 线程ID
    pthread_t m_thread;            // pthread线程句柄
    std::function<void()> m_cb;    // 线程执行的回调函数
    std::string m_name;            // 线程名称
    Semaphore m_semaphore;         // 信号量，用于线程同步
    
private:
    static void* run(void* arg);
    
public:
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    
    pid_t getId() const;
    const std::string& getName() const;
    void join();
    
    static pid_t GetThreadId();
    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
};
```

### 2.2 线程模型

mycoroutine 库的线程模型基于 POSIX 线程库（pthread），提供了面向对象的封装。每个 `Thread` 对象代表一个线程，包含线程的ID、名称、回调函数等信息。

线程的创建流程：
1. 创建 `Thread` 对象，指定回调函数和名称
2. 在构造函数中调用 `pthread_create()` 创建线程
3. 线程启动后，执行 `run()` 静态函数
4. `run()` 函数调用用户指定的回调函数
5. 回调函数执行完毕后，线程结束

### 2.3 信号量实现

信号量是一种经典的线程同步机制，用于控制对共享资源的访问。`Semaphore` 类实现了标准的 P/V 操作：

- **P 操作（wait）**：尝试获取资源，如果资源不可用则阻塞等待
- **V 操作（signal）**：释放资源，通知等待的线程

信号量的实现基于 C++11 的 `std::mutex` 和 `std::condition_variable`，确保线程安全和正确的同步行为。

## 3. API 接口说明

### 3.1 Semaphore 类接口

#### 3.1.1 构造函数

**功能**：创建信号量对象

**参数**：
- `count_`：信号量的初始值，默认为 0

**使用示例**：
```cpp
// 创建初始值为1的信号量（互斥锁）
mycoroutine::Semaphore sem(1);

// 创建初始值为0的信号量（事件）
mycoroutine::Semaphore event;
```

#### 3.1.2 wait()

**功能**：P 操作，尝试获取资源

**说明**：
- 如果 `count > 0`，则获取资源，`count` 减 1
- 如果 `count == 0`，则阻塞等待，直到有资源可用
- 使用 `while` 循环避免虚假唤醒

**使用示例**：
```cpp
// 尝试获取资源
sem.wait();
try {
    // 访问共享资源
    shared_resource->do_something();
} catch (...) {
    // 处理异常
    sem.signal();
    throw;
}
// 释放资源
sem.signal();
```

#### 3.1.3 signal()

**功能**：V 操作，释放资源

**说明**：
- 释放资源，`count` 加 1
- 通知一个等待的线程

**使用示例**：
```cpp
// 生产资源
shared_resource->produce();
// 通知消费者
sem.signal();
```

### 3.2 Thread 类接口

#### 3.2.1 构造函数

**功能**：创建并启动线程

**参数**：
- `cb`：线程要执行的回调函数
- `name`：线程名称

**使用示例**：
```cpp
// 创建线程，执行lambda函数
mycoroutine::Thread thread([]() {
    std::cout << "Hello from thread" << std::endl;
}, "TestThread");
```

#### 3.2.2 析构函数

**功能**：线程析构函数

**说明**：
- 确保线程正确结束
- 如果线程仍在运行，则分离它

#### 3.2.3 getId()

**功能**：获取线程ID

**返回值**：线程的系统ID（`pid_t`）

**使用示例**：
```cpp
std::cout << "Thread ID: " << thread.getId() << std::endl;
```

#### 3.2.4 getName()

**功能**：获取线程名称

**返回值**：线程名称的引用

**使用示例**：
```cpp
std::cout << "Thread Name: " << thread.getName() << std::endl;
```

#### 3.2.5 join()

**功能**：等待线程结束

**说明**：
- 阻塞当前线程，直到被join的线程执行完毕
- 只能调用一次，多次调用会导致未定义行为

**使用示例**：
```cpp
// 创建线程
mycoroutine::Thread thread([]() {
    // 执行耗时操作
    sleep(2);
    std::cout << "Thread finished" << std::endl;
}, "JoinThread");

// 等待线程结束
std::cout << "Waiting for thread to finish..." << std::endl;
thread.join();
std::cout << "Thread joined" << std::endl;
```

#### 3.2.6 静态方法

##### 3.2.6.1 GetThreadId()

**功能**：获取当前线程的系统ID

**返回值**：当前线程的系统ID（`pid_t`）

**使用示例**：
```cpp
std::cout << "Current Thread ID: " << mycoroutine::Thread::GetThreadId() << std::endl;
```

##### 3.2.6.2 GetThis()

**功能**：获取当前线程对象

**返回值**：当前线程对象的指针

**使用示例**：
```cpp
mycoroutine::Thread* current_thread = mycoroutine::Thread::GetThis();
if (current_thread) {
    std::cout << "Current Thread Name: " << current_thread->getName() << std::endl;
}
```

##### 3.2.6.3 GetName()

**功能**：获取当前线程的名称

**返回值**：当前线程名称的引用

**使用示例**：
```cpp
std::cout << "Current Thread Name: " << mycoroutine::Thread::GetName() << std::endl;
```

##### 3.2.6.4 SetName(const std::string& name)

**功能**：设置当前线程的名称

**参数**：
- `name`：要设置的线程名称

**使用示例**：
```cpp
// 设置当前线程名称
mycoroutine::Thread::SetName("NewThreadName");
```

## 4. 实现原理

### 4.1 线程创建与启动

线程的创建和启动流程如下：

1. 创建 `Thread` 对象，指定回调函数和名称
2. 在构造函数中，初始化成员变量
3. 调用 `pthread_create()` 创建线程，指定线程函数为 `run()` 静态方法
4. `run()` 方法执行以下操作：
   a. 设置当前线程对象
   b. 调用用户指定的回调函数
   c. 清理线程资源

### 4.2 线程同步机制

`Thread` 类使用 `Semaphore` 信号量实现线程同步，主要用于线程创建过程中的同步：

1. 在构造函数中，创建信号量（初始值为0）
2. 调用 `pthread_create()` 创建线程
3. 调用 `semaphore.wait()` 等待线程启动
4. 在线程函数 `run()` 中，设置当前线程对象后，调用 `semaphore.signal()` 通知构造函数线程已启动

### 4.3 线程局部存储

线程管理模块使用线程局部存储（Thread Local Storage, TLS）存储当前线程对象的指针。这样，在任何线程中都可以通过 `Thread::GetThis()` 获取当前线程对象。

### 4.4 信号量实现原理

`Semaphore` 类基于 C++11 的 `std::mutex` 和 `std::condition_variable` 实现：

- `mtx` 互斥锁用于保护 `count` 变量的访问
- `cv` 条件变量用于线程等待和唤醒
- `count` 变量表示可用资源的数量

`wait()` 方法的实现：
```cpp
void Semaphore::wait() {
    std::unique_lock<std::mutex> lock(mtx);
    // 使用while避免虚假唤醒
    while (count == 0) {
        cv.wait(lock); // 等待signal信号
    }
    count--; // 获取资源，计数器减1
}
```

`signal()` 方法的实现：
```cpp
void Semaphore::signal() {
    std::unique_lock<std::mutex> lock(mtx);
    count++; // 释放资源，计数器加1
    cv.notify_one(); // 通知一个等待的线程
}
```

## 5. 使用示例

### 5.1 基本线程使用

```cpp
#include <iostream>
#include <mycoroutine/thread.h>

using namespace mycoroutine;

int main() {
    std::cout << "Main thread ID: " << Thread::GetThreadId() << std::endl;
    
    // 创建线程
    Thread thread([]() {
        std::cout << "Child thread ID: " << Thread::GetThreadId() << std::endl;
        std::cout << "Child thread name: " << Thread::GetName() << std::endl;
    }, "ChildThread");
    
    // 等待线程结束
    thread.join();
    
    std::cout << "Main thread finished" << std::endl;
    
    return 0;
}
```

### 5.2 信号量作为互斥锁

```cpp
#include <iostream>
#include <vector>
#include <mycoroutine/thread.h>

using namespace mycoroutine;

class Counter {
private:
    int m_count = 0;
    Semaphore m_mutex{1}; // 初始值为1的信号量，用作互斥锁
    
public:
    void increment() {
        m_mutex.wait(); // 获取锁
        m_count++;
        m_mutex.signal(); // 释放锁
    }
    
    int get() {
        m_mutex.wait(); // 获取锁
        int val = m_count;
        m_mutex.signal(); // 释放锁
        return val;
    }
};

int main() {
    Counter counter;
    
    // 创建多个线程同时递增计数器
    std::vector<std::unique_ptr<Thread>> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(new Thread([&counter]() {
            for (int j = 0; j < 10000; ++j) {
                counter.increment();
            }
        }, "Thread" + std::to_string(i)));
    }
    
    // 等待所有线程结束
    for (auto& thread : threads) {
        thread->join();
    }
    
    std::cout << "Final count: " << counter.get() << std::endl;
    
    return 0;
}
```

### 5.3 信号量作为事件

```cpp
#include <iostream>
#include <mycoroutine/thread.h>

using namespace mycoroutine;

int main() {
    Semaphore event;
    int shared_data = 0;
    
    // 创建生产者线程
    Thread producer([&event, &shared_data]() {
        std::cout << "Producer: Producing data..." << std::endl;
        sleep(2); // 模拟生产过程
        shared_data = 42;
        std::cout << "Producer: Data produced, notifying consumer" << std::endl;
        event.signal(); // 通知消费者数据已生产
    }, "Producer");
    
    // 创建消费者线程
    Thread consumer([&event, &shared_data]() {
        std::cout << "Consumer: Waiting for data..." << std::endl;
        event.wait(); // 等待数据生产完成
        std::cout << "Consumer: Data received: " << shared_data << std::endl;
    }, "Consumer");
    
    // 等待线程结束
    producer.join();
    consumer.join();
    
    return 0;
}
```

## 6. 性能优化

### 6.1 线程创建开销

- 线程创建是一个相对昂贵的操作，建议使用线程池复用线程
- 避免频繁创建和销毁线程
- 对于短任务，考虑使用协程替代线程

### 6.2 信号量性能

- 信号量基于互斥锁和条件变量实现，有一定的开销
- 对于高并发场景，考虑使用更轻量级的同步机制
- 避免不必要的信号量操作

### 6.3 线程局部存储

- 线程局部存储的访问速度比全局变量慢，但比互斥锁快
- 合理使用线程局部存储，避免滥用

## 7. 注意事项

### 7.1 线程安全

- `Thread` 类的方法不是线程安全的，不要在多个线程中同时操作同一个 `Thread` 对象
- 线程的回调函数中要注意线程安全，避免竞态条件

### 7.2 资源管理

- 确保线程执行完毕后释放所有资源
- 避免线程泄漏（创建线程后不等待或分离）

### 7.3 异常处理

- 线程回调函数中抛出的异常会被忽略，建议在回调函数内部处理异常
- 确保异常不会导致资源泄漏

### 7.4 线程名称

- 线程名称的长度有限制（通常为16个字符）
- 过长的线程名称会被截断

### 7.5 线程ID

- 线程ID在进程内唯一，但在系统范围内不唯一
- 线程ID的类型是 `pid_t`，但实际上是线程的TID（Thread ID）

## 8. 总结

线程管理模块是 mycoroutine 库的基础组件，提供了易用的线程创建、管理和同步机制。该模块包含两个主要类：

- `Semaphore`：信号量类，用于线程间同步，实现了经典的P/V操作
- `Thread`：线程类，封装了pthread线程库，提供了面向对象的线程操作接口

线程管理模块具有以下特点：

- 易用性：提供简洁的API接口，隐藏底层实现细节
- 可靠性：确保线程的正确创建和销毁
- 安全性：提供线程安全的操作
- 跨平台：基于标准C++11和pthread，支持多种Linux发行版

该模块为上层的协程调度器、IO管理器等组件提供了基础支持，是构建高性能并发应用的重要基础。