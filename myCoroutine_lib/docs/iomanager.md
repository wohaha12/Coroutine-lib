# IO管理器模块 (IOManager)

## 1. 模块概述

IO管理器是 mycoroutine 库的核心组件之一，负责处理异步 IO 事件。该模块基于 Linux 的 `epoll` 机制实现了高效的 IO 多路复用，支持读写事件的监控和处理，同时集成了定时器功能，可以处理超时事件。IO 管理器继承自调度器，能够将 IO 事件的处理与协程调度紧密结合，实现高效的异步 IO 编程模型。

### 1.1 主要功能
- IO 事件监控：支持对文件描述符的读写事件进行监控
- 事件回调：当 IO 事件就绪时，自动调度协程或执行回调函数
- 定时器功能：集成了定时器管理器，支持超时事件处理
- 线程唤醒机制：使用 `eventfd` 实现高效的线程唤醒
- 事件管理：支持事件的添加、删除、取消和触发
- 高效的事件循环：基于 `epoll` 实现的高效事件循环

### 1.2 设计目标
- 高性能：使用 `epoll` 机制，支持大量并发连接
- 低延迟：事件就绪后立即调度处理，减少延迟
- 易用性：提供简洁的 API 接口，易于使用
- 可靠性：完善的错误处理和资源管理
- 扩展性：支持自定义事件处理逻辑

## 2. 核心设计

### 2.1 类继承关系

```
+-----------------+
|   TimerManager  |
+-----------------+
        ^
        |
+-----------------+     +-----------------+
|    Scheduler    |     |                 |
+-----------------+     |                 |
        ^               |                 |
        |               |                 |
+-----------------+     |                 |
|    IOManager    +-----+                 |
+-----------------+     |                 |
        |               |                 |
        |               |                 |
        v               v                 v
+-----------------+ +-----------------+ +-----------------+
|   事件循环      | |   定时器管理    | |   协程调度      |
+-----------------+ +-----------------+ +-----------------+
```

IOManager 同时继承自 `Scheduler` 和 `TimerManager`，具有以下功能：
- 从 `Scheduler` 继承：协程调度、线程池管理、任务队列
- 从 `TimerManager` 继承：定时器管理、超时事件处理

### 2.2 核心数据结构

#### 2.2.1 事件类型枚举

```cpp
enum Event {
    NONE = 0x0,     // 无事件
    READ = 0x1,     // 读事件，对应 EPOLLIN
    WRITE = 0x4     // 写事件，对应 EPOLLOUT
};
```

事件类型使用位掩码表示，可以组合使用（如 `READ | WRITE`）。

#### 2.2.2 文件描述符上下文

```cpp
struct FdContext {
    struct EventContext {
        Scheduler *scheduler = nullptr;  // 事件所属的调度器
        std::shared_ptr<Fiber> fiber;    // 事件触发时要执行的协程
        std::function<void()> cb;        // 事件触发时要执行的回调函数
    };
    
    EventContext read;      // 读事件上下文
    EventContext write;     // 写事件上下文
    int fd = 0;             // 文件描述符
    Event events = NONE;    // 当前注册的事件
    std::mutex mutex;       // 用于保护该结构体的互斥锁
    
    // 方法声明
};
```

每个文件描述符对应一个 `FdContext` 对象，管理该文件描述符的所有事件和回调信息。

### 2.3 事件处理流程

```
1. 用户调用 addEvent() 添加 IO 事件监控
2. IOManager 检查文件描述符上下文是否存在，不存在则创建
3. 设置事件上下文（协程或回调函数）
4. 将事件添加到 epoll 中
5. 当 IO 事件就绪时，epoll_wait() 返回
6. 遍历就绪事件列表
7. 触发对应的事件处理函数
8. 重置事件上下文
9. 调度协程或执行回调函数
```

### 2.4 线程唤醒机制

IOManager 使用 `eventfd` 作为线程唤醒机制，比传统的 `pipe` 更高效：
- 占用更少的文件描述符（只需要一个 `eventfd`，而 `pipe` 需要两个）
- 支持原子操作，不需要额外的同步机制
- 更高效的系统调用，减少系统开销

当需要唤醒工作线程时，IOManager 会向 `eventfd` 写入一个 64 位整数，`epoll_wait()` 会立即返回，唤醒线程处理新的任务或事件。

## 3. API 接口说明

### 3.1 构造与析构

#### 3.1.1 IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "IOManager")

**功能**：创建 IO 管理器

**参数**：
- `threads`：工作线程数量，默认为 1
- `use_caller`：是否将调用者线程作为工作线程，默认为 true
- `name`：IO 管理器名称，默认为 "IOManager"

**返回值**：无

**使用示例**：
```cpp
// 创建一个包含2个额外线程的IO管理器，使用调用者线程
auto iomanager = std::make_shared<mycoroutine::IOManager>(2, true, "MyIOManager");
```

#### 3.1.2 ~IOManager()

**功能**：IO 管理器析构函数，释放资源

**参数**：无

**返回值**：无

### 3.2 IO 事件管理

#### 3.2.1 int addEvent(int fd, Event event, std::function<void()> cb = nullptr)

**功能**：添加 IO 事件监控

**参数**：
- `fd`：文件描述符
- `event`：事件类型（READ、WRITE 或组合）
- `cb`：事件回调函数，默认为 nullptr（使用当前协程）

**返回值**：成功返回 0，失败返回 -1

**说明**：当指定 `cb` 时，事件就绪时执行回调函数；否则，事件就绪时恢复当前协程执行。

**使用示例**：
```cpp
// 使用回调函数监控读事件
iomanager->addEvent(fd, mycoroutine::IOManager::READ, [fd](){
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    // 处理读取的数据
});

// 使用当前协程监控写事件
iomanager->addEvent(fd, mycoroutine::IOManager::WRITE);
```

#### 3.2.2 bool delEvent(int fd, Event event)

**功能**：删除 IO 事件监控

**参数**：
- `fd`：文件描述符
- `event`：事件类型（READ、WRITE 或组合）

**返回值**：成功返回 true，失败返回 false

**说明**：删除指定文件描述符的指定事件，但不触发回调函数。

**使用示例**：
```cpp
// 删除读事件监控
iomanager->delEvent(fd, mycoroutine::IOManager::READ);
```

#### 3.2.3 bool cancelEvent(int fd, Event event)

**功能**：取消 IO 事件监控并触发回调

**参数**：
- `fd`：文件描述符
- `event`：事件类型（READ、WRITE 或组合）

**返回值**：成功返回 true，失败返回 false

**说明**：取消指定文件描述符的指定事件，并触发对应的回调函数。

**使用示例**：
```cpp
// 取消写事件监控并触发回调
iomanager->cancelEvent(fd, mycoroutine::IOManager::WRITE);
```

#### 3.2.4 bool cancelAll(int fd)

**功能**：取消文件描述符上的所有事件监控并触发回调

**参数**：
- `fd`：文件描述符

**返回值**：成功返回 true，失败返回 false

**说明**：取消指定文件描述符上的所有事件，并触发对应的回调函数。

**使用示例**：
```cpp
// 取消文件描述符上的所有事件
iomanager->cancelAll(fd);
```

### 3.3 静态方法

#### 3.3.1 static IOManager* GetThis()

**功能**：获取当前线程的 IO 管理器实例

**参数**：无

**返回值**：当前线程的 IO 管理器指针

**使用示例**：
```cpp
// 获取当前IO管理器
auto iomanager = mycoroutine::IOManager::GetThis();
```

### 3.4 保护成员函数

#### 3.4.1 void tickle() override

**功能**：唤醒线程函数

**参数**：无

**返回值**：无

**说明**：当有新任务或事件添加时，唤醒空闲线程。

#### 3.4.2 bool stopping() override

**功能**：判断 IO 管理器是否可以停止

**参数**：无

**返回值**：可以停止返回 true，否则返回 false

**说明**：检查是否有未处理的事件或定时器，决定是否可以停止。

#### 3.4.3 void idle() override

**功能**：空闲线程执行的函数

**参数**：无

**返回值**：无

**说明**：当没有任务或事件时，执行空闲处理逻辑，主要是调用 `epoll_wait()` 等待事件就绪。

#### 3.4.4 void onTimerInsertedAtFront() override

**功能**：定时器插入到队首时的回调

**参数**：无

**返回值**：无

**说明**：当新添加的定时器超时时间比当前所有定时器都早时，调用该函数，唤醒线程重新计算超时时间。

## 4. 实现原理

### 4.1 事件循环

IOManager 的核心是基于 `epoll` 的事件循环，主要在 `idle()` 函数中实现：

```cpp
void IOManager::idle() {
    // 创建epoll事件数组
    epoll_event* events = new epoll_event[64]();
    
    while (!stopping()) {
        // 计算超时时间
        uint64_t next_timeout = getNextTimer();
        
        // 调用epoll_wait等待事件就绪
        int rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
        
        // 处理就绪事件
        if (rt > 0) {
            for (int i = 0; i < rt; ++i) {
                // 处理事件
                if (events[i].data.fd == m_tickleFds) {
                    // 线程唤醒事件，读取eventfd内容
                    uint64_t dummy;
                    read(m_tickleFds, &dummy, sizeof(dummy));
                } else {
                    // IO事件，触发对应的事件处理
                    triggerEvent(events[i].data.fd, events[i].events);
                }
            }
        } else if (rt == 0) {
            // 超时事件，处理定时器
            tickleTimer();
        }
        
        // 处理队列中的任务
        auto cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->yield();
    }
    
    delete[] events;
}
```

### 4.2 事件管理

IOManager 使用文件描述符上下文数组 `m_fdContexts` 管理所有文件描述符的事件信息：

1. **添加事件**：调用 `addEvent()` 时，IOManager 会：
   - 检查文件描述符上下文是否存在，不存在则创建
   - 设置事件上下文（协程或回调函数）
   - 将事件添加到 `epoll` 中
   - 更新文件描述符的事件状态

2. **删除事件**：调用 `delEvent()` 时，IOManager 会：
   - 从 `epoll` 中删除事件
   - 重置事件上下文
   - 更新文件描述符的事件状态

3. **取消事件**：调用 `cancelEvent()` 时，IOManager 会：
   - 从 `epoll` 中删除事件
   - 触发事件回调
   - 重置事件上下文
   - 更新文件描述符的事件状态

4. **触发事件**：当事件就绪时，IOManager 会：
   - 检查事件上下文
   - 如果是协程，调度协程执行
   - 如果是回调函数，执行回调函数
   - 重置事件上下文

### 4.3 线程唤醒机制

IOManager 使用 `eventfd` 实现高效的线程唤醒：

1. 在构造函数中创建 `eventfd`：
   ```cpp
   m_tickleFds = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
   ```

2. 将 `eventfd` 添加到 `epoll` 中，监听读事件

3. 当需要唤醒线程时，向 `eventfd` 写入一个 64 位整数：
   ```cpp
   uint64_t one = 1;
   write(m_tickleFds, &one, sizeof(one));
   ```

4. `epoll_wait()` 会立即返回，唤醒线程处理新的任务或事件

### 4.4 定时器集成

IOManager 集成了定时器管理器，支持超时事件处理：

1. 在 `idle()` 函数中，计算下一个定时器的超时时间
2. 将超时时间作为 `epoll_wait()` 的超时参数
3. 如果 `epoll_wait()` 超时，调用 `tickleTimer()` 处理超时的定时器
4. 当新的定时器插入到队首时，调用 `onTimerInsertedAtFront()` 唤醒线程，重新计算超时时间

## 5. 使用示例

### 5.1 简单的 TCP 服务器

```cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

void handle_client(int client_fd, IOManager* iom) {
    std::cout << "New client connected: " << client_fd << std::endl;
    
    char buf[1024];
    while (true) {
        // 监听读事件
        iom->addEvent(client_fd, IOManager::READ);
        
        // 读取数据
        int n = read(client_fd, buf, sizeof(buf));
        if (n <= 0) {
            std::cout << "Client disconnected: " << client_fd << std::endl;
            break;
        }
        
        // 回显数据
        buf[n] = '\0';
        std::cout << "Received: " << buf << std::endl;
        
        // 监听写事件
        iom->addEvent(client_fd, IOManager::WRITE);
        write(client_fd, buf, n);
    }
    
    close(client_fd);
}

void tcp_server() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // 设置地址复用
    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    if (bind(sock_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind address" << std::endl;
        close(sock_fd);
        return;
    }
    
    // 监听端口
    if (listen(sock_fd, 1024) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        close(sock_fd);
        return;
    }
    
    std::cout << "TCP server started on port 8080" << std::endl;
    
    auto iom = IOManager::GetThis();
    
    while (true) {
        // 监听读事件（accept）
        iom->addEvent(sock_fd, IOManager::READ);
        
        // 接受连接
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(sock_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // 设置非阻塞
        fcntl(client_fd, F_SETFL, O_NONBLOCK);
        
        // 创建协程处理客户端连接
        iom->scheduleLock([client_fd, iom]() {
            handle_client(client_fd, iom);
        });
    }
    
    close(sock_fd);
}

int main() {
    // 创建IO管理器，使用4个线程
    IOManager iom(3, true, "TCPServerIOManager");
    
    // 启动TCP服务器
    iom.scheduleLock(tcp_server);
    
    // 运行IO管理器
    iom.start();
    
    return 0;
}
```

### 5.2 定时器使用

```cpp
#include <iostream>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

int main() {
    // 创建IO管理器
    IOManager iom(1, true, "TimerIOManager");
    
    // 添加定时器，500ms后执行
    iom.addTimer(500, []() {
        std::cout << "Timer 500ms triggered" << std::endl;
    });
    
    // 添加周期性定时器，每隔1000ms执行一次，执行5次
    iom.addTimer(1000, []() {
        static int count = 0;
        std::cout << "Periodic timer 1000ms triggered, count: " << ++count << std::endl;
        return count < 5;
    });
    
    // 启动IO管理器
    iom.start();
    
    return 0;
}
```

## 6. 性能优化

### 6.1 事件循环优化

- 使用 `epoll` 机制，支持大量并发连接
- 合理设置 `epoll_wait()` 的超时时间，平衡 CPU 使用率和响应延迟
- 事件数组大小适中，避免频繁的内存分配

### 6.2 线程唤醒机制

- 使用 `eventfd` 替代 `pipe`，减少系统开销
- 只有当队列从空变为非空时，才唤醒线程，避免不必要的唤醒
- 唤醒操作是原子的，不需要额外的同步机制

### 6.3 事件管理

- 使用数组存储文件描述符上下文，访问速度快
- 动态调整上下文数组大小，避免浪费内存
- 事件触发时，批量处理就绪事件，减少锁的持有时间

### 6.4 定时器优化

- 使用红黑树管理定时器，插入和删除操作的时间复杂度为 O(log n)
- 只在定时器插入到队首时，才重新计算超时时间，减少系统调用
- 批量处理超时事件，提高效率

## 7. 注意事项

### 7.1 文件描述符管理

- 确保文件描述符设置为非阻塞模式，否则会导致线程阻塞
- 及时关闭不再使用的文件描述符，避免资源泄漏
- 不要在多个 IO 管理器中同时使用同一个文件描述符

### 7.2 线程安全

- IOManager 的 API 接口是线程安全的，可以从任意线程调用
- 但同一个文件描述符的事件操作应该在同一个线程中执行，避免竞争条件

### 7.3 事件处理

- 避免在事件回调函数中执行长时间的阻塞操作，会影响其他事件的处理
- 对于耗时的操作，应该创建新的协程处理
- 注意处理事件触发后的状态重置，避免重复触发

### 7.4 错误处理

- 检查 API 调用的返回值，及时处理错误
- 处理 `epoll` 错误，如 `EINTR` 等
- 确保资源正确释放，避免内存泄漏

## 8. 总结

IOManager 是 mycoroutine 库的核心组件，实现了高效的 IO 事件处理机制。该模块基于 `epoll` 实现了高效的 IO 多路复用，支持读写事件的监控和处理，同时集成了定时器功能，可以处理超时事件。IOManager 继承自调度器，能够将 IO 事件的处理与协程调度紧密结合，实现高效的异步 IO 编程模型。

IOManager 的设计具有以下特点：

- 高性能：使用 `epoll` 机制，支持大量并发连接
- 低延迟：事件就绪后立即调度处理，减少延迟
- 易用性：提供简洁的 API 接口，易于使用
- 可靠性：完善的错误处理和资源管理
- 扩展性：支持自定义事件处理逻辑

该模块为上层应用提供了高效的异步 IO 处理能力，是构建高性能网络应用的重要基础。