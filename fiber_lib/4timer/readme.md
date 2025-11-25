# 定时器模块（Timer）

## 项目简介

定时器模块是协程库中的重要组件，提供高效的定时器管理功能，支持一次性定时器、循环定时器和条件定时器。该模块基于时间堆实现，能够精确管理大量定时器，并高效触发超时回调。

## 核心功能

- **高精度定时器**：基于系统时钟实现的高精度定时器功能
- **定时器类型**：支持一次性定时器、循环定时器和条件定时器
- **高效管理**：基于std::set实现的最小堆结构，高效管理大量定时器
- **线程安全**：使用读写锁保证在多线程环境下的安全访问
- **时钟回退检测**：能够检测系统时钟回退并进行处理
- **可继承扩展**：TimerManager类设计为可继承的，允许子类扩展功能

## 项目结构

```
4timer/
├── readme.md     # 项目说明文档
├── test.cpp      # 测试程序
├── timer.cpp     # Timer和TimerManager类的实现
└── timer.h       # 类定义和接口声明
```

## 编译说明

### 编译命令

```bash
g++ *.cpp -std=c++17 -o test
```

### 编译要求

- 支持C++17标准的编译器
- 标准库支持（STL）
- 无需额外依赖

## 调试开关

如需启用调试日志，可在编译时添加宏定义：

```bash
g++ *.cpp -std=c++17 -DDEBUG -o test
```

## 类和API说明

### Timer类

表示单个定时器对象，封装了定时器的属性和操作。

#### 公共方法

```cpp
// 取消定时器
// @return 取消成功返回true，失败返回false
bool cancel();

// 刷新定时器，将下次超时时间调整为当前时间+超时时间
// @return 刷新成功返回true，失败返回false
bool refresh();

// 重设定时器的超时时间
// @param ms 新的超时时间（毫秒）
// @param from_now 是否从当前时间开始计算
// @return 重置成功返回true，失败返回false
bool reset(uint64_t ms, bool from_now);
```

### TimerManager类

定时器管理器，负责管理多个定时器，是该模块的核心类。

#### 公共方法

```cpp
// 添加定时器
// @param ms 超时时间（毫秒）
// @param cb 超时回调函数
// @param recurring 是否循环执行，默认false
// @return 创建的定时器智能指针
std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

// 添加条件定时器
// @param ms 超时时间（毫秒）
// @param cb 超时回调函数
// @param weak_cond 条件对象的弱引用
// @param recurring 是否循环执行，默认false
// @return 创建的定时器智能指针
std::shared_ptr<Timer> addConditionTimer(uint64_t ms, std::function<void()> cb,
                                         std::weak_ptr<void> weak_cond, bool recurring = false);

// 获取下一个定时器的超时时间
// @return 下一个定时器的剩余超时时间（毫秒），无定时器时返回最大值
uint64_t getNextTimer();

// 获取所有过期的定时器回调函数
// @param cbs 存储过期定时器回调函数的容器
void listExpiredCb(std::vector<std::function<void()>>& cbs);

// 检查管理器中是否有定时器
// @return 有定时器返回true，否则返回false
bool hasTimer();
```

#### 保护方法（供子类重写）

```cpp
// 当定时器插入到堆顶时的回调
// 子类可重写此方法，用于唤醒等待的线程等操作
virtual void onTimerInsertedAtFront() {};
```

## 工作原理

### 时间堆实现

定时器管理器使用std::set作为底层数据结构，实现了一个最小堆，按定时器的下一次超时时间排序。这样可以快速获取最近要超时的定时器。

### 定时器触发机制

1. 调用`getNextTimer()`获取下一个定时器的剩余超时时间
2. 等待该时间后，调用`listExpiredCb()`收集所有过期的定时器回调
3. 执行收集到的回调函数
4. 对于循环定时器，自动重新计算下一次超时时间并重新插入时间堆

### 条件定时器

条件定时器使用std::weak_ptr引用条件对象，在超时触发回调前会检查条件对象是否还存在。如果条件对象已被销毁，则不会执行回调函数，避免悬挂指针问题。

## 使用示例

### 基本用法

```cpp
#include "timer.h"

void test_timer() {
    // 创建定时器管理器
    sylar::TimerManager timerMgr;
    
    // 添加一次性定时器（2000毫秒后执行）
    timerMgr.addTimer(2000, []() {
        std::cout << "一次性定时器触发" << std::endl;
    });
    
    // 添加循环定时器（每1000毫秒执行一次）
    timerMgr.addTimer(1000, []() {
        std::cout << "循环定时器触发" << std::endl;
    }, true);
    
    // 条件定时器示例
    auto condition = std::make_shared<int>(42);
    timerMgr.addConditionTimer(3000, []() {
        std::cout << "条件定时器触发" << std::endl;
    }, condition);
    
    // 主循环：检查并处理超时定时器
    while (true) {
        uint64_t next = timerMgr.getNextTimer();
        if (next != ~0ull) {
            // 等待到下一个定时器超时
            std::this_thread::sleep_for(std::chrono::milliseconds(next));
        } else {
            // 无定时器时休眠一小段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // 收集并执行过期的回调函数
        std::vector<std::function<void()>> cbs;
        timerMgr.listExpiredCb(cbs);
        for (auto& cb : cbs) {
            cb();
        }
    }
}
```

## 测试程序说明

`test.cpp`文件包含了定时器模块的完整测试示例，演示了以下功能：

1. 一次性定时器的创建和触发
2. 循环定时器的创建和周期性触发
3. 定时器的取消操作
4. 定时器管理器的基本使用流程

运行编译后的测试程序，可以观察到各种定时器的触发行为。

## 注意事项

1. **线程安全**：TimerManager提供了线程安全的接口，但回调函数的执行需要调用者自行保证线程安全
2. **回调函数**：回调函数应该尽量简洁，避免阻塞操作，以确保定时器系统的高效运行
3. **循环定时器**：循环定时器的回调执行时间应小于定时时间，否则会导致定时器累积
4. **条件定时器**：条件定时器依赖于弱引用的有效性，注意管理好条件对象的生命周期
5. **系统时间**：模块会检测系统时间回退，但过于频繁的时间调整可能影响定时器精度

## 性能考虑

1. **定时器数量**：该实现能够高效管理大量定时器，时间复杂度为O(log n)的查找和插入
2. **超时处理**：使用批量收集回调函数的方式，减少锁的竞争
3. **读写锁**：使用std::shared_mutex允许并发读取定时器信息，提高并发性能
4. **内存管理**：使用智能指针自动管理定时器对象的生命周期

## 扩展建议

1. 可以继承TimerManager实现更复杂的定时器调度策略
2. 考虑添加定时器优先级支持
3. 可以实现基于刻度的定时器（Timer Wheel）以进一步提高大量短时间定时器的性能
4. 对于高频率触发的定时器，可以考虑合并或批处理优化