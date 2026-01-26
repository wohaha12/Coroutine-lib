# 文件描述符管理模块 (FD Manager)

## 1. 模块概述

文件描述符管理模块是 mycoroutine 库的重要组件，负责管理和跟踪所有文件描述符的状态和属性。该模块提供了文件描述符上下文的创建、初始化、查询和删除功能，支持非阻塞设置、超时控制等特性，为系统调用钩子和 IO 管理器提供基础支持。

### 1.1 主要功能
- 文件描述符上下文的创建和管理
- 非阻塞状态的设置和查询（系统层面和用户层面）
- 套接字识别
- 接收和发送超时时间的设置和查询
- 线程安全的访问机制
- 单例模式的管理器实现

### 1.2 设计目标
- 统一管理所有文件描述符的状态
- 支持透明的非阻塞 IO 操作
- 提供灵活的超时控制机制
- 线程安全的访问接口
- 高效的文件描述符上下文查询

## 2. 核心设计

### 2.1 类结构设计

```cpp
// 文件描述符上下文类
class FdCtx : public std::enable_shared_from_this<FdCtx> {
private:
    bool m_isInit = false;       // 是否已初始化
    bool m_isSocket = false;     // 是否为套接字
    bool m_sysNonblock = false;  // 系统层面是否非阻塞
    bool m_userNonblock = false; // 用户层面是否非阻塞
    bool m_isClosed = false;     // 是否已关闭
    int m_fd;                    // 文件描述符值
    
    uint64_t m_recvTimeout = (uint64_t)-1; // 接收超时时间
    uint64_t m_sendTimeout = (uint64_t)-1; // 发送超时时间
    
public:
    FdCtx(int fd);
    ~FdCtx();
    
    bool init();
    bool isInit() const;
    bool isSocket() const;
    bool isClosed() const;
    
    void setUserNonblock(bool v);
    bool getUserNonblock() const;
    
    void setSysNonblock(bool v);
    bool getSysNonblock() const;
    
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);
};

// 文件描述符管理器类
class FdManager {
private:
    std::shared_mutex m_mutex;                        // 读写锁
    std::vector<std::shared_ptr<FdCtx>> m_datas;      // 文件描述符上下文数组
    
public:
    FdManager();
    std::shared_ptr<FdCtx> get(int fd, bool auto_create = false);
    void del(int fd);
};

// 单例模板类
template<typename T>
class Singleton {
private:
    static T* instance;
    static std::mutex mutex;
    
protected:
    Singleton() {};
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static T* GetInstance();
    static void DestroyInstance();
};

// 文件描述符管理器单例类型别名
typedef Singleton<FdManager> FdMgr;
```

### 2.2 设计理念

文件描述符管理模块采用了以下设计理念：

1. **上下文封装**：将每个文件描述符的状态和属性封装到 `FdCtx` 对象中，便于统一管理和查询。

2. **分层非阻塞机制**：支持系统层面和用户层面的非阻塞设置，实现透明的非阻塞 IO 操作。

3. **灵活的超时控制**：为每个文件描述符的接收和发送操作设置独立的超时时间，支持灵活的超时控制。

4. **高效的查询机制**：使用数组存储文件描述符上下文，通过文件描述符值直接索引，实现 O(1) 时间复杂度的查询。

5. **线程安全访问**：使用读写锁保护共享数据，支持多线程并发访问。

6. **单例模式**：文件描述符管理器采用单例模式实现，确保全局只有一个管理器实例。

### 2.3 非阻塞机制

文件描述符管理模块实现了分层的非阻塞机制：

- **系统层面非阻塞**：通过 `fcntl` 系统调用设置文件描述符的非阻塞标志，影响系统调用的行为。
- **用户层面非阻塞**：由用户显式设置，表示用户希望以非阻塞方式使用该文件描述符，系统调用钩子会根据此标志决定是否需要挂起协程。

这种分层设计允许系统调用钩子根据具体情况灵活处理 IO 操作，实现透明的非阻塞 IO。

### 2.4 超时控制机制

文件描述符管理模块为每个文件描述符的接收和发送操作提供了独立的超时控制：

- **接收超时**：设置 `recv()`、`read()` 等读取操作的超时时间。
- **发送超时**：设置 `send()`、`write()` 等写入操作的超时时间。

超时时间以毫秒为单位，默认为 `-1`（表示永不超时）。系统调用钩子会根据这些超时设置，在 IO 操作超时时自动取消操作并返回超时错误。

## 3. API 接口说明

### 3.1 FdCtx 类接口

#### 3.1.1 构造函数

**功能**：创建文件描述符上下文对象

**参数**：
- `fd`：文件描述符值

**使用示例**：
```cpp
// 创建文件描述符上下文
mycoroutine::FdCtx fd_ctx(fd);
```

#### 3.1.2 析构函数

**功能**：销毁文件描述符上下文对象

#### 3.1.3 init()

**功能**：初始化文件描述符上下文

**返回值**：初始化成功返回 `true`，否则返回 `false`

**说明**：
- 检查文件描述符是否有效
- 确定文件描述符是否为套接字
- 设置初始的非阻塞状态

**使用示例**：
```cpp
if (!fd_ctx.init()) {
    std::cerr << "Failed to initialize fd context" << std::endl;
    return -1;
}
```

#### 3.1.4 isInit()

**功能**：获取文件描述符是否已初始化

**返回值**：已初始化返回 `true`，否则返回 `false`

**使用示例**：
```cpp
if (!fd_ctx.isInit()) {
    if (!fd_ctx.init()) {
        // 初始化失败处理
    }
}
```

#### 3.1.5 isSocket()

**功能**：获取文件描述符是否为套接字

**返回值**：是套接字返回 `true`，否则返回 `false`

**使用示例**：
```cpp
if (fd_ctx.isSocket()) {
    // 套接字特定处理
} else {
    // 普通文件描述符处理
}
```

#### 3.1.6 isClosed()

**功能**：获取文件描述符是否已关闭

**返回值**：已关闭返回 `true`，否则返回 `false`

**使用示例**：
```cpp
if (fd_ctx.isClosed()) {
    std::cerr << "File descriptor already closed" << std::endl;
    return -1;
}
```

#### 3.1.7 setUserNonblock(bool v)

**功能**：设置用户层面的非阻塞状态

**参数**：
- `v`：是否设置为非阻塞

**使用示例**：
```cpp
// 设置用户层面非阻塞
fd_ctx.setUserNonblock(true);
```

#### 3.1.8 getUserNonblock()

**功能**：获取用户层面的非阻塞状态

**返回值**：非阻塞返回 `true`，否则返回 `false`

**使用示例**：
```cpp
if (fd_ctx.getUserNonblock()) {
    // 非阻塞模式处理
} else {
    // 阻塞模式处理
}
```

#### 3.1.9 setSysNonblock(bool v)

**功能**：设置系统层面的非阻塞状态

**参数**：
- `v`：是否设置为非阻塞

**说明**：
- 调用 `fcntl` 系统调用设置文件描述符的非阻塞标志
- 影响系统调用的实际行为

**使用示例**：
```cpp
// 设置系统层面非阻塞
fd_ctx.setSysNonblock(true);
```

#### 3.1.10 getSysNonblock()

**功能**：获取系统层面的非阻塞状态

**返回值**：非阻塞返回 `true`，否则返回 `false`

**使用示例**：
```cpp
bool is_nonblock = fd_ctx.getSysNonblock();
```

#### 3.1.11 setTimeout(int type, uint64_t v)

**功能**：设置文件描述符的超时时间

**参数**：
- `type`：超时类型（`SO_RCVTIMEO` 表示接收超时，`SO_SNDTIMEO` 表示发送超时）
- `v`：超时时间（毫秒），`-1` 表示永不超时

**使用示例**：
```cpp
// 设置接收超时为1000ms
fd_ctx.setTimeout(SO_RCVTIMEO, 1000);

// 设置发送超时为2000ms
fd_ctx.setTimeout(SO_SNDTIMEO, 2000);
```

#### 3.1.12 getTimeout(int type)

**功能**：获取文件描述符的超时时间

**参数**：
- `type`：超时类型（`SO_RCVTIMEO` 表示接收超时，`SO_SNDTIMEO` 表示发送超时）

**返回值**：超时时间（毫秒），`-1` 表示永不超时

**使用示例**：
```cpp
// 获取接收超时时间
uint64_t recv_timeout = fd_ctx.getTimeout(SO_RCVTIMEO);

// 获取发送超时时间
uint64_t send_timeout = fd_ctx.getTimeout(SO_SNDTIMEO);
```

### 3.2 FdManager 类接口

#### 3.2.1 构造函数

**功能**：创建文件描述符管理器对象

**说明**：
- 初始化文件描述符上下文数组

#### 3.2.2 get(int fd, bool auto_create = false)

**功能**：获取文件描述符对应的上下文对象

**参数**：
- `fd`：文件描述符值
- `auto_create`：如果上下文不存在，是否自动创建

**返回值**：文件描述符上下文智能指针，如果不存在且 `auto_create` 为 `false`，则返回 `nullptr`

**使用示例**：
```cpp
// 获取文件描述符上下文，不存在则自动创建
auto fd_ctx = mycoroutine::FdMgr::GetInstance()->get(fd, true);

// 获取文件描述符上下文，不存在则返回nullptr
auto fd_ctx = mycoroutine::FdMgr::GetInstance()->get(fd, false);
if (!fd_ctx) {
    // 上下文不存在处理
}
```

#### 3.2.3 del(int fd)

**功能**：删除文件描述符对应的上下文对象

**参数**：
- `fd`：文件描述符值

**使用示例**：
```cpp
// 删除文件描述符上下文
mycoroutine::FdMgr::GetInstance()->del(fd);
```

### 3.3 Singleton 模板类

**功能**：提供线程安全的单例实现

**使用示例**：
```cpp
// 获取文件描述符管理器单例实例
auto fd_mgr = mycoroutine::FdMgr::GetInstance();

// 获取文件描述符上下文
auto fd_ctx = fd_mgr->get(fd, true);
```

## 4. 实现原理

### 4.1 文件描述符上下文管理

文件描述符管理器使用动态扩展的数组存储文件描述符上下文对象。当需要获取某个文件描述符的上下文时，直接通过文件描述符值作为数组索引查找对应的上下文对象。如果数组大小不足，会自动扩展数组大小以容纳新的文件描述符。

### 4.2 非阻塞设置实现

**系统层面非阻塞**：
- 通过 `fcntl` 系统调用设置文件描述符的 `O_NONBLOCK` 标志
- 影响所有使用该文件描述符的系统调用行为
- 系统调用在无法立即完成时会返回 `-1` 并设置 `errno` 为 `EAGAIN` 或 `EWOULDBLOCK`

**用户层面非阻塞**：
- 仅作为标记，不直接影响系统调用行为
- 由系统调用钩子根据该标志决定是否需要挂起协程
- 实现透明的非阻塞 IO 操作

### 4.3 超时控制实现

文件描述符上下文存储了接收和发送操作的超时时间，但实际的超时检测由系统调用钩子和 IO 管理器实现：

1. 系统调用钩子在执行 IO 操作前，获取文件描述符的超时设置
2. 如果操作无法立即完成且设置了超时时间，系统调用钩子会将当前协程挂起，并向 IO 管理器注册 IO 事件
3. IO 管理器在检测到 IO 事件就绪或超时时，唤醒对应的协程
4. 协程恢复执行后，系统调用钩子会检查是否超时，并返回相应的错误

### 4.4 线程安全实现

文件描述符管理模块使用以下机制确保线程安全：

- `FdManager` 类使用 `std::shared_mutex` 保护共享的文件描述符上下文数组
- 读操作（`get` 方法）使用共享锁，允许多个线程同时读取
- 写操作（`del` 方法和上下文初始化）使用排他锁，确保数据一致性
- `Singleton` 模板类使用互斥锁保护单例实例的创建

## 5. 使用示例

### 5.1 基本使用

```cpp
#include <iostream>
#include <mycoroutine/fd_manager.h>

using namespace mycoroutine;

int main() {
    // 创建文件描述符
    int fd = open("test.txt", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        std::cerr << "Failed to open file" << std::endl;
        return -1;
    }
    
    // 获取文件描述符上下文，自动创建
    auto fd_ctx = FdMgr::GetInstance()->get(fd, true);
    if (!fd_ctx) {
        std::cerr << "Failed to get fd context" << std::endl;
        close(fd);
        return -1;
    }
    
    // 初始化上下文
    if (!fd_ctx->init()) {
        std::cerr << "Failed to init fd context" << std::endl;
        close(fd);
        return -1;
    }
    
    // 设置非阻塞
    fd_ctx->setSysNonblock(true);
    fd_ctx->setUserNonblock(true);
    
    // 设置超时时间
    fd_ctx->setTimeout(SO_RCVTIMEO, 1000);
    fd_ctx->setTimeout(SO_SNDTIMEO, 2000);
    
    // 获取状态
    std::cout << "FD: " << fd << std::endl;
    std::cout << "Is socket: " << (fd_ctx->isSocket() ? "yes" : "no") << std::endl;
    std::cout << "Sys nonblock: " << (fd_ctx->getSysNonblock() ? "yes" : "no") << std::endl;
    std::cout << "User nonblock: " << (fd_ctx->getUserNonblock() ? "yes" : "no") << std::endl;
    std::cout << "Recv timeout: " << fd_ctx->getTimeout(SO_RCVTIMEO) << "ms" << std::endl;
    std::cout << "Send timeout: " << fd_ctx->getTimeout(SO_SNDTIMEO) << "ms" << std::endl;
    
    // 关闭文件描述符
    close(fd);
    
    // 删除上下文
    FdMgr::GetInstance()->del(fd);
    
    return 0;
}
```

### 5.2 与系统调用钩子结合使用

```cpp
// 系统调用钩子中的典型用法
ssize_t my_read(int fd, void* buf, size_t count) {
    // 获取文件描述符上下文
    auto fd_ctx = mycoroutine::FdMgr::GetInstance()->get(fd, true);
    if (!fd_ctx || !fd_ctx->init()) {
        // 使用原始系统调用
        return ::read(fd, buf, count);
    }
    
    // 检查用户是否希望非阻塞
    if (!fd_ctx->getUserNonblock()) {
        // 使用原始系统调用
        return ::read(fd, buf, count);
    }
    
    // 设置系统非阻塞
    if (!fd_ctx->getSysNonblock()) {
        fd_ctx->setSysNonblock(true);
    }
    
    // 尝试读取数据
    ssize_t n = ::read(fd, buf, count);
    if (n >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return n;
    }
    
    // IO 操作无法立即完成，需要挂起协程
    // 向 IO 管理器注册读事件
    // 挂起当前协程
    // ...
    
    // 协程被唤醒后，重新尝试读取
    return ::read(fd, buf, count);
}
```

## 6. 性能优化

### 6.1 上下文查询优化

- 使用数组存储文件描述符上下文，实现 O(1) 时间复杂度的查询
- 动态扩展数组大小，避免不必要的内存分配
- 使用智能指针管理上下文对象，避免内存泄漏

### 6.2 锁竞争优化

- 使用读写锁分离读操作和写操作，提高并发性能
- 读操作（`get` 方法）使用共享锁，允许多个线程同时读取
- 写操作（`del` 方法和上下文初始化）使用排他锁，确保数据一致性

### 6.3 延迟初始化

- 文件描述符上下文在首次使用时才初始化，避免不必要的系统调用
- 初始化过程中只进行必要的系统调用，减少开销

## 7. 注意事项

### 7.1 文件描述符有效性

- 文件描述符管理模块不会主动检查文件描述符的有效性，需要用户自行确保
- 关闭文件描述符后，应及时调用 `del` 方法删除对应的上下文
- 重用文件描述符时，会创建新的上下文对象，不会影响之前的上下文

### 7.2 非阻塞状态管理

- 系统层面和用户层面的非阻塞状态是独立的，需要分别设置
- 系统调用钩子会根据用户层面的非阻塞标志决定是否需要挂起协程
- 直接调用系统调用时，实际的行为由系统层面的非阻塞标志决定

### 7.3 超时设置

- 文件描述符上下文的超时设置只是存储超时时间，实际的超时检测由系统调用钩子和 IO 管理器实现
- 不同的系统调用可能有不同的超时处理机制，需要根据具体情况使用
- 超时时间以毫秒为单位，默认为 `-1`（表示永不超时）

### 7.4 线程安全

- 文件描述符管理模块的 API 接口是线程安全的，可以从任意线程调用
- 但同一个文件描述符上下文的方法不是线程安全的，需要用户自行确保线程安全

### 7.5 资源管理

- 文件描述符管理模块不会主动关闭文件描述符，需要用户自行管理
- 关闭文件描述符后，应及时删除对应的上下文对象，释放资源

## 8. 总结

文件描述符管理模块是 mycoroutine 库的重要组件，提供了统一的文件描述符管理机制。该模块通过封装文件描述符的状态和属性，实现了透明的非阻塞 IO 操作和灵活的超时控制，为系统调用钩子和 IO 管理器提供了基础支持。

文件描述符管理模块具有以下特点：

- 高效的文件描述符上下文查询（O(1) 时间复杂度）
- 分层的非阻塞机制，支持系统层面和用户层面的非阻塞设置
- 灵活的超时控制，为每个文件描述符的接收和发送操作设置独立的超时时间
- 线程安全的访问接口，支持多线程并发访问
- 单例模式的管理器实现，确保全局只有一个管理器实例

该模块的设计和实现为 mycoroutine 库的高效 IO 操作和协程调度提供了坚实的基础，是构建高性能网络应用的重要组件。