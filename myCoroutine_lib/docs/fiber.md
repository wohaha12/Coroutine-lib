# 协程核心模块 (Fiber)

## 1. 模块概述

协程核心模块是 mycoroutine 库的基础组件，负责协程的创建、切换、状态管理和资源释放。该模块基于 Linux 系统的 `ucontext_t` 实现了高效的用户级协程，支持协程的创建、恢复、让出和销毁等核心功能。

### 1.1 主要功能
- 协程的创建与销毁
- 协程上下文的保存与恢复
- 协程状态管理
- 主协程与子协程的支持
- 协程栈的动态分配与释放
- 协程的重置与重用

### 1.2 设计目标
- 高性能：协程切换开销小
- 易用性：提供简洁的 API 接口
- 安全性：完善的资源管理机制
- 灵活性：支持多种使用场景

## 2. 核心设计

### 2.1 协程状态机

协程具有三种状态，状态转换关系如下：

```
+----------+      resume()      +----------+
|  READY   +------------------->+ RUNNING  |
+----------+                    +----------+
       ^                            |
       |                            |
       | yield() or                 |
       | complete                   |
       +----------------------------+
       |                            |
       |                            v
+----------+      reset()       +----------+
|  TERM    <-------------------+          |
+----------+                    +----------+
```

- **READY**：协程已创建但尚未运行，或已让出执行权等待再次调度
- **RUNNING**：协程正在执行中
- **TERM**：协程执行完毕，或已被重置

### 2.2 协程结构设计

```cpp
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    enum State { READY, RUNNING, TERM };
    
private:
    Fiber();  // 私有构造函数，用于创建主协程
    
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);
    ~Fiber();
    
    void reset(std::function<void()> cb);
    void resume();
    void yield();
    
    uint64_t getId() const;
    State getState() const;
    
public:
    static void SetThis(Fiber *f);
    static std::shared_ptr<Fiber> GetThis();
    static void SetSchedulerFiber(Fiber* f);
    static uint64_t GetFiberId();
    static void MainFunc();
    
private:
    uint64_t m_id = 0;            // 协程ID
    uint32_t m_stacksize = 0;     // 协程栈大小
    State m_state = READY;        // 协程状态
    ucontext_t m_ctx;             // 协程上下文
    void* m_stack = nullptr;      // 协程栈指针
    std::function<void()> m_cb;   // 协程回调函数
    bool m_runInScheduler;        // 是否在调度器中运行
    std::mutex m_mutex;           // 协程互斥锁
};
```

### 2.3 主协程与子协程

- **主协程**：每个线程默认创建的协程，使用线程的默认栈空间，负责调度和管理其他协程
- **子协程**：用户显式创建的协程，拥有独立的栈空间，执行具体的任务

### 2.4 协程栈管理

- **栈分配**：子协程创建时分配独立的栈空间，默认大小为 128KB
- **栈保护**：未实现栈溢出保护（可扩展）
- **栈释放**：协程销毁时释放栈空间，避免内存泄漏
- **栈重用**：支持通过 `reset()` 方法重用已终止的协程栈空间

## 3. API 接口说明

### 3.1 构造与析构

#### 3.1.1 Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true)

**功能**：创建一个新的子协程

**参数**：
- `cb`：协程要执行的回调函数
- `stacksize`：协程栈大小，默认为 0（使用默认大小 128KB）
- `run_in_scheduler`：是否在调度器中运行，默认为 true

**返回值**：无

**使用示例**：
```cpp
// 创建一个协程，执行 lambda 表达式
auto fiber = std::make_shared<Fiber>([](){
    std::cout << "Hello, Fiber!" << std::endl;
});
```

#### 3.1.2 ~Fiber()

**功能**：协程析构函数，释放协程资源

**参数**：无

**返回值**：无

### 3.2 协程操作

#### 3.2.1 void reset(std::function<void()> cb)

**功能**：重置协程，设置新的回调函数

**参数**：
- `cb`：新的协程回调函数

**返回值**：无

**说明**：仅当协程处于 TERM 状态时可以调用，用于重用协程栈空间

**使用示例**：
```cpp
// 重置已终止的协程
fiber->reset([](){
    std::cout << "Reset Fiber!" << std::endl;
});
```

#### 3.2.2 void resume()

**功能**：恢复协程执行

**参数**：无

**返回值**：无

**说明**：只能恢复处于 READY 状态的协程，调用后协程状态变为 RUNNING

**使用示例**：
```cpp
// 恢复协程执行
fiber->resume();
```

#### 3.2.3 void yield()

**功能**：协程让出执行权

**参数**：无

**返回值**：无

**说明**：协程主动让出执行权，状态变为 READY，控制权交回调用者

**使用示例**：
```cpp
// 协程内部主动让出执行权
void fiber_func() {
    std::cout << "Before yield" << std::endl;
    mycoroutine::Fiber::GetThis()->yield();
    std::cout << "After yield" << std::endl;
}
```

### 3.3 协程信息获取

#### 3.3.1 uint64_t getId() const

**功能**：获取协程 ID

**参数**：无

**返回值**：协程的唯一标识符

#### 3.3.2 State getState() const

**功能**：获取协程状态

**参数**：无

**返回值**：协程当前状态（READY、RUNNING 或 TERM）

### 3.4 静态方法

#### 3.4.1 static std::shared_ptr<Fiber> GetThis()

**功能**：获取当前运行的协程

**参数**：无

**返回值**：当前运行协程的智能指针

**说明**：如果当前没有协程在运行，则创建一个主协程

**使用示例**：
```cpp
// 获取当前协程
auto current_fiber = mycoroutine::Fiber::GetThis();
```

#### 3.4.2 static uint64_t GetFiberId()

**功能**：获取当前运行的协程 ID

**参数**：无

**返回值**：当前协程的 ID，如果没有协程运行则返回 -1

## 4. 实现原理

### 4.1 协程切换机制

协程切换基于 Linux 系统的 `ucontext_t` 机制实现，主要涉及以下函数：

- `getcontext()`：获取当前上下文
- `setcontext()`：设置当前上下文
- `makecontext()`：创建新的上下文
- `swapcontext()`：交换两个上下文

### 4.2 协程创建流程

1. 分配协程栈空间
2. 初始化协程上下文 `ucontext_t`
3. 设置协程入口函数为 `MainFunc()`
4. 将用户回调函数保存到协程对象中
5. 设置协程状态为 READY

### 4.3 协程恢复流程

1. 检查协程状态，必须为 READY
2. 设置当前运行协程为该协程
3. 更新协程状态为 RUNNING
4. 使用 `swapcontext()` 切换到协程上下文
5. 协程执行完毕后，自动切换回调用者

### 4.4 协程让出流程

1. 检查协程状态，必须为 RUNNING
2. 更新协程状态为 READY
3. 保存当前协程上下文
4. 使用 `swapcontext()` 切换回调用者上下文

### 4.5 协程入口函数

所有协程的执行都通过 `MainFunc()` 入口，该函数负责：

1. 执行用户回调函数
2. 处理回调函数抛出的异常
3. 执行完毕后，将协程状态设置为 TERM
4. 切换回调用者协程

## 5. 使用示例

### 5.1 简单协程使用

```cpp
#include <iostream>
#include <mycoroutine/fiber.h>

int main() {
    // 获取主协程
    auto main_fiber = mycoroutine::Fiber::GetThis();
    
    // 创建子协程
    auto fiber = std::make_shared<mycoroutine::Fiber>([](){
        std::cout << "Fiber start" << std::endl;
        
        // 让出执行权
        mycoroutine::Fiber::GetThis()->yield();
        
        std::cout << "Fiber resume" << std::endl;
    });
    
    std::cout << "Main start" << std::endl;
    
    // 恢复子协程执行
    fiber->resume();
    
    std::cout << "Main resume after yield" << std::endl;
    
    // 再次恢复子协程执行
    fiber->resume();
    
    std::cout << "Main end" << std::endl;
    
    return 0;
}
```

**输出结果**：
```
Main start
Fiber start
Main resume after yield
Fiber resume
Main end
```

### 5.2 协程重置与重用

```cpp
#include <iostream>
#include <mycoroutine/fiber.h>

int main() {
    auto fiber = std::make_shared<mycoroutine::Fiber>([](){
        std::cout << "First execution" << std::endl;
    });
    
    fiber->resume();
    
    // 重置协程
    fiber->reset([](){
        std::cout << "Second execution" << std::endl;
    });
    
    fiber->resume();
    
    return 0;
}
```

**输出结果**：
```
First execution
Second execution
```

## 6. 性能优化

### 6.1 协程栈大小优化

- 默认栈大小为 128KB，可以根据实际需求调整
- 对于内存敏感的应用，可以减小栈大小
- 对于栈使用量大的应用，需要适当增大栈大小

### 6.2 协程重用

- 使用 `reset()` 方法重用已终止的协程，可以减少内存分配和释放开销
- 适合创建大量短期存在的协程场景

### 6.3 避免频繁切换

- 协程切换虽然比线程切换快，但仍然有一定开销
- 设计时应避免不必要的协程切换
- 合理组织任务，减少上下文切换次数

## 7. 注意事项

### 7.1 协程状态管理

- 不要在协程外部直接修改协程状态
- 只能通过 `resume()` 和 `yield()` 方法改变协程状态
- 调用 `reset()` 方法前，确保协程已处于 TERM 状态

### 7.2 栈溢出问题

- 协程栈空间有限，避免在协程中使用大量栈空间
- 避免在协程中创建大型局部变量
- 避免深度递归调用

### 7.3 线程安全

- 协程对象不是线程安全的，不要在多个线程中同时操作同一个协程
- 可以使用 `m_mutex` 成员进行同步，但应尽量避免
- 每个线程有自己的协程调度器和主协程

## 8. 总结

协程核心模块是 mycoroutine 库的基础，实现了高效的用户级协程功能。该模块基于 `ucontext_t` 机制，支持协程的创建、切换、恢复和销毁，具有以下特点：

- 高性能：协程切换开销小，适合高并发场景
- 易用性：提供简洁的 API 接口，易于使用
- 安全性：使用智能指针管理生命周期，避免资源泄漏
- 灵活性：支持主协程和子协程，支持协程重用

该模块为上层的协程调度器、IO 管理器等组件提供了基础支持，是整个协程库的核心组件。