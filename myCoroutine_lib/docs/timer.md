# 定时器模块 (Timer)

## 1. 模块概述

定时器模块是 mycoroutine 库的重要组件，负责管理和调度定时任务。该模块提供了高效的定时器实现，支持一次性定时器、循环定时器和条件定时器，能够准确地在指定时间点执行回调函数。定时器模块被 IO 管理器等组件使用，用于处理超时事件和定时任务。

### 1.1 主要功能
- 支持一次性定时器和循环定时器
- 支持定时器的取消、刷新和重置操作
- 支持条件定时器，当条件不再满足时不会执行回调
- 高效的定时器管理，基于有序集合实现
- 系统时钟回退检测，确保定时器准确性
- 支持批量获取过期定时器的回调函数

### 1.2 设计目标
- 高精度：尽可能准确地执行定时任务
- 高效：支持大量定时器，插入、删除、查询操作的时间复杂度为 O(log n)
- 可靠：处理系统时钟回退，确保定时器不会异常触发
- 易用：提供简洁的 API 接口，易于使用
- 可扩展：支持自定义定时器管理策略

## 2. 核心设计

### 2.1 类关系

```
+-----------------+
|  TimerManager   |
+-----------------+
        ^
        | 包含
        v
+-----------------+
|     Timer       |
+-----------------+
        |
        v
+-----------------+
|   回调函数      |
+-----------------+
```

### 2.2 定时器结构设计

```cpp
class Timer : public std::enable_shared_from_this<Timer> {
public:
    bool cancel();
    bool refresh();
    bool reset(uint64_t ms, bool from_now);
    
private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
    
private:
    bool m_recurring = false;       // 是否循环执行
    uint64_t m_ms = 0;              // 超时时间（毫秒）
    std::chrono::time_point<std::chrono::system_clock> m_next;  // 下一次超时时间
    std::function<void()> m_cb;     // 回调函数
    TimerManager* m_manager = nullptr;  // 所属的定时器管理器
    
    struct Comparator {
        bool operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const;
    };
};

class TimerManager {
public:
    TimerManager();
    virtual ~TimerManager();
    
    std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    std::shared_ptr<Timer> addConditionTimer(uint64_t ms, std::function<void()> cb, 
                                             std::weak_ptr<void> weak_cond, bool recurring = false);
    
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void()>>& cbs);
    bool hasTimer();
    
protected:
    virtual void onTimerInsertedAtFront() {};
    
private:
    bool detectClockRollover();
    void addTimer(std::shared_ptr<Timer> timer);
    
private:
    std::shared_mutex m_mutex;  // 保护定时器集合
    std::set<std::shared_ptr<Timer>, Timer::Comparator> m_timers;  // 定时器集合
    bool m_tickled = false;  // 标志位，指示是否需要唤醒线程
    std::chrono::time_point<std::chrono::system_clock> m_previouseTime;  // 上次检查时间
};
```

### 2.3 定时器比较器

定时器使用 `Timer::Comparator` 进行比较，比较规则是：

1. 如果两个定时器的下一次超时时间不同，则超时时间早的定时器排在前面
2. 如果两个定时器的下一次超时时间相同，则指针地址小的定时器排在前面

### 2.4 系统时钟回退检测

定时器管理器会检测系统时钟是否回退，避免因系统时间调整导致定时器异常触发。检测方法是：

1. 保存上次检查时间 `m_previouseTime`
2. 每次获取当前时间时，比较与上次检查时间的关系
3. 如果当前时间小于上次检查时间，说明系统时钟发生了回退
4. 回退处理：将所有定时器的下一次超时时间调整为当前时间加上定时时间

## 3. API 接口说明

### 3.1 Timer 类接口

#### 3.1.1 cancel()

**功能**：取消定时器

**返回值**：取消成功返回 true，失败返回 false

**说明**：
- 从定时器管理器中移除该定时器
- 清空回调函数
- 只能取消正在等待执行的定时器

#### 3.1.2 refresh()

**功能**：刷新定时器，将下一次超时时间调整为当前时间加上定时时间

**返回值**：刷新成功返回 true，失败返回 false

**说明**：
- 适用于需要重置定时器超时时间的场景
- 保持定时器的其他属性不变

#### 3.1.3 reset(uint64_t ms, bool from_now)

**功能**：重设定时器

**参数**：
- `ms`：新的超时时间（毫秒）
- `from_now`：是否从当前时间开始计算超时时间

**返回值**：重置成功返回 true，失败返回 false

**说明**：
- 修改定时器的超时时间
- 可以选择从当前时间或原超时时间开始计算

### 3.2 TimerManager 类接口

#### 3.2.1 addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false)

**功能**：添加定时器

**参数**：
- `ms`：超时时间（毫秒）
- `cb`：超时回调函数
- `recurring`：是否循环执行，默认为 false

**返回值**：创建的定时器智能指针

**说明**：
- 创建定时器对象
- 将定时器添加到管理器的定时器集合中
- 如果定时器是第一个定时器，调用 `onTimerInsertedAtFront()` 回调

#### 3.2.2 addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false)

**功能**：添加条件定时器

**参数**：
- `ms`：超时时间（毫秒）
- `cb`：超时回调函数
- `weak_cond`：条件对象的弱引用
- `recurring`：是否循环执行，默认为 false

**返回值**：创建的定时器智能指针

**说明**：
- 创建条件定时器，当条件不再满足时不会执行回调
- 使用 `weak_ptr` 避免内存泄漏
- 条件对象被销毁时，定时器自动取消

#### 3.2.3 getNextTimer()

**功能**：获取下一个定时器的剩余超时时间

**返回值**：下一个定时器的剩余超时时间（毫秒），如果没有定时器则返回最大值

**说明**：
- 计算当前时间到下一个定时器超时时间的差值
- 处理系统时钟回退情况
- 用于 `epoll_wait()` 的超时参数设置

#### 3.2.4 listExpiredCb(std::vector<std::function<void()>>& cbs)

**功能**：获取所有过期定时器的回调函数

**参数**：
- `cbs`：用于存储过期定时器回调函数的容器

**返回值**：无

**说明**：
- 遍历定时器集合，查找所有已超时的定时器
- 将过期定时器的回调函数添加到 `cbs` 中
- 对于循环定时器，更新其下一次超时时间
- 对于一次性定时器，从集合中移除

#### 3.2.5 hasTimer()

**功能**：判断管理器中是否有定时器

**返回值**：有定时器返回 true，否则返回 false

**说明**：
- 检查定时器集合是否为空
- 线程安全的查询操作

### 3.3 保护成员函数

#### 3.3.1 onTimerInsertedAtFront()

**功能**：当定时器插入到集合首位置时的回调

**说明**：
- 虚函数，允许派生类重写
- 用于唤醒等待的线程，重新计算超时时间
- IO 管理器等组件会重写该函数

## 4. 实现原理

### 4.1 定时器管理机制

定时器管理器使用 `std::set` 实现的有序集合来管理定时器，集合中的定时器按超时时间排序。`std::set` 是基于红黑树实现的，因此插入、删除、查询操作的时间复杂度均为 O(log n)。

### 4.2 定时器添加流程

1. 创建 `Timer` 对象，设置超时时间和回调函数
2. 获取当前时间，计算下一次超时时间
3. 将定时器添加到有序集合中
4. 如果定时器是第一个定时器，调用 `onTimerInsertedAtFront()` 回调

### 4.3 定时器过期处理流程

1. 调用 `listExpiredCb()` 获取所有过期定时器的回调函数
2. 遍历定时器集合，查找所有超时时间小于等于当前时间的定时器
3. 对于每个过期定时器：
   a. 如果是循环定时器，更新其下一次超时时间，继续留在集合中
   b. 如果是一次性定时器，从集合中移除
   c. 将回调函数添加到输出容器中
4. 返回所有过期定时器的回调函数

### 4.4 系统时钟回退处理

1. 在 `getNextTimer()` 和 `listExpiredCb()` 中检测系统时钟回退
2. 如果检测到系统时钟回退：
   a. 记录当前时间为新的基准时间
   b. 遍历所有定时器，重新计算下一次超时时间
   c. 确保定时器不会因为系统时钟回退而异常触发

### 4.5 条件定时器实现

条件定时器的实现原理：

1. 创建一个包装回调函数，该函数会：
   a. 尝试将 `weak_ptr` 升级为 `shared_ptr`
   b. 如果升级成功，执行原始回调函数
   c. 如果升级失败，说明条件对象已销毁，不执行回调
2. 将包装后的回调函数设置为定时器的回调
3. 当条件对象销毁时，`weak_ptr` 升级失败，定时器自动失效

## 5. 使用示例

### 5.1 一次性定时器

```cpp
#include <iostream>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

int main() {
    IOManager iom(1, true, "TimerTest");
    
    // 添加一次性定时器，500ms后执行
    iom.addTimer(500, []() {
        std::cout << "One-shot timer triggered" << std::endl;
    });
    
    iom.start();
    
    return 0;
}
```

### 5.2 循环定时器

```cpp
#include <iostream>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

int main() {
    IOManager iom(1, true, "PeriodicTimerTest");
    
    // 添加循环定时器，每隔1000ms执行一次，执行5次后停止
    iom.addTimer(1000, []() {
        static int count = 0;
        std::cout << "Periodic timer triggered, count: " << ++count << std::endl;
        return count < 5;  // 返回false时停止循环
    });
    
    iom.start();
    
    return 0;
}
```

### 5.3 条件定时器

```cpp
#include <iostream>
#include <memory>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

class MyClass {
public:
    MyClass() {
        std::cout << "MyClass constructed" << std::endl;
    }
    
    ~MyClass() {
        std::cout << "MyClass destructed" << std::endl;
    }
    
    void doSomething() {
        std::cout << "MyClass::doSomething() called" << std::endl;
    }
};

int main() {
    IOManager iom(1, true, "ConditionTimerTest");
    
    // 创建条件对象
    auto obj = std::make_shared<MyClass>();
    
    // 添加条件定时器，当obj被销毁时，定时器不再执行
    iom.addConditionTimer(500, [obj]() {
        obj->doSomething();
    }, obj);
    
    // 5秒后销毁obj
    iom.addTimer(5000, [&iom, obj]() {
        std::cout << "Destroying obj" << std::endl;
        // obj离开作用域后自动销毁
    });
    
    iom.start();
    
    return 0;
}
```

### 5.4 定时器的取消与刷新

```cpp
#include <iostream>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

int main() {
    IOManager iom(1, true, "TimerControlTest");
    
    // 创建定时器
    auto timer = iom.addTimer(1000, []() {
        std::cout << "Timer triggered" << std::endl;
    });
    
    // 200ms后取消定时器
    iom.addTimer(200, [timer]() {
        std::cout << "Cancelling timer" << std::endl;
        timer->cancel();
    });
    
    // 300ms后刷新定时器（如果已取消则无效）
    iom.addTimer(300, [timer]() {
        std::cout << "Refreshing timer" << std::endl;
        timer->refresh();
    });
    
    // 400ms后重设定时器
    iom.addTimer(400, [timer]() {
        std::cout << "Resetting timer" << std::endl;
        timer->reset(500, true);
    });
    
    iom.start();
    
    return 0;
}
```

## 6. 性能优化

### 6.1 定时器管理优化

- 使用 `std::set` 实现有序集合，确保插入、删除、查询操作的时间复杂度为 O(log n)
- 使用 `shared_mutex` 支持并发读写，提高并发性能
- 批量获取过期定时器的回调函数，减少锁的持有时间

### 6.2 系统时钟回退处理

- 检测系统时钟回退，避免定时器异常触发
- 回退处理时，只调整受影响的定时器，减少不必要的计算

### 6.3 条件定时器优化

- 使用 `weak_ptr` 避免内存泄漏
- 条件对象销毁时，自动停止定时器，无需手动取消
- 包装回调函数，避免异常传播

### 6.4 定时器粒度

- 定时器的最小粒度为毫秒级，适合大多数应用场景
- 避免设置过小的超时时间，减少系统开销
- 对于高精度定时需求，建议使用硬件定时器或其他专门的定时机制

## 7. 注意事项

### 7.1 回调函数处理

- 定时器回调函数中不要执行长时间阻塞的操作，会影响其他定时器的执行
- 回调函数中抛出的异常会被忽略，建议自行处理
- 回调函数执行完毕后，定时器会自动处理后续逻辑

### 7.2 内存管理

- 使用智能指针管理定时器对象，避免内存泄漏
- 条件定时器使用 `weak_ptr` 避免循环引用
- 定时器被取消或执行完毕后，会自动清理资源

### 7.3 线程安全

- 定时器管理器的 API 接口是线程安全的，可以从任意线程调用
- 定时器的回调函数在 IO 管理器的线程中执行
- 避免在多个线程中同时操作同一个定时器

### 7.4 系统时钟回退

- 定时器管理器会检测系统时钟回退，但仍建议避免频繁调整系统时间
- 系统时钟回退可能导致定时器延迟执行
- 对于时间敏感的应用，建议使用单调时钟

### 7.5 定时器精度

- 定时器的精度受系统时钟精度和调度延迟的影响
- 实际执行时间可能会比设定时间稍晚
- 对于高精度定时需求，建议使用其他专门的定时机制

## 8. 总结

定时器模块是 mycoroutine 库的重要组件，提供了高效、可靠的定时器管理功能。该模块基于有序集合实现，支持大量定时器的管理，具有以下特点：

- 高效的定时器管理，插入、删除、查询操作的时间复杂度为 O(log n)
- 支持一次性定时器、循环定时器和条件定时器
- 系统时钟回退检测，确保定时器准确性
- 简洁易用的 API 接口
- 可扩展的设计，允许自定义定时器管理策略

定时器模块被 IO 管理器等组件使用，用于处理超时事件和定时任务，是构建高性能网络应用的重要基础。