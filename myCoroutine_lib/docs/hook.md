# 系统调用钩子模块 (Hook)

## 1. 模块概述

系统调用钩子模块是 mycoroutine 库的核心组件之一，负责拦截和转换系统调用，将阻塞式的系统调用转换为非阻塞的协程挂起操作。该模块通过动态链接和函数重定向技术，实现了透明的非阻塞 IO 操作，允许用户使用熟悉的阻塞式 API 编写异步程序，同时获得异步 IO 的性能优势。

### 1.1 主要功能
- 拦截和转换常见的系统调用
- 将阻塞式 IO 转换为非阻塞的协程挂起
- 支持睡眠函数的协程化
- 透明的 API 设计，无需修改现有代码
- 可动态启用和禁用钩子
- 支持多种系统调用，包括网络、文件 IO 和套接字操作

### 1.2 设计目标
- 透明性：用户无需修改现有代码即可享受非阻塞 IO 带来的性能优势
- 高性能：最小化钩子带来的额外开销
- 可靠性：确保系统调用的正确性和一致性
- 灵活性：支持动态启用和禁用钩子
- 可扩展性：支持添加新的系统调用钩子

## 2. 核心设计

### 2.1 设计原理

系统调用钩子模块的设计基于以下原理：

1. **函数重定向**：通过 LD_PRELOAD 或其他动态链接技术，将系统调用函数替换为钩子函数
2. **原始函数保存**：保存原始系统调用函数的指针，以便在需要时调用
3. **非阻塞转换**：将阻塞式系统调用转换为非阻塞操作，结合 IO 管理器实现协程挂起和唤醒
4. **透明 API**：保持与原始系统调用相同的函数签名和行为，确保现有代码无需修改
5. **动态控制**：支持动态启用和禁用钩子，方便调试和性能优化

### 2.2 钩子函数分类

系统调用钩子模块支持以下几类系统调用：

| 类别 | 系统调用 |
|-----|--------|
| 睡眠函数 | sleep, usleep, nanosleep |
| 网络函数 | socket, connect, accept |
| 读取函数 | read, readv, recv, recvfrom, recvmsg |
| 写入函数 | write, writev, send, sendto, sendmsg |
| 文件描述符函数 | close |
| 文件控制函数 | fcntl, ioctl |
| 套接字选项函数 | getsockopt, setsockopt |

### 2.3 核心数据结构

```cpp
// 系统调用钩子开关控制
bool is_hook_enable();
void set_hook_enable(bool flag);

// 原始系统调用函数指针类型定义
typedef unsigned int (*sleep_fun) (unsigned int seconds);
typedef int (*usleep_fun) (useconds_t usec);
typedef int (*nanosleep_fun) (const struct timespec* req, struct timespec* rem);
typedef int (*socket_fun) (int domain, int type, int protocol);
typedef int (*connect_fun) (int sockfd, const struct sockaddr *addr, socklen_t addrlen);
typedef int (*accept_fun) (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef ssize_t (*read_fun) (int fd, void *buf, size_t count);
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*recv_fun) (int sockfd, void *buf, size_t len, int flags);
typedef ssize_t (*recvfrom_fun) (int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
typedef ssize_t (*recvmsg_fun) (int sockfd, struct msghdr *msg, int flags);
typedef ssize_t (*write_fun) (int fd, const void *buf, size_t count);
typedef ssize_t (*writev_fun) (int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t (*send_fun) (int sockfd, const void *buf, size_t len, int flags);
typedef ssize_t (*sendto_fun) (int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
typedef ssize_t (*sendmsg_fun) (int sockfd, const struct msghdr *msg, int flags);
typedef int (*close_fun) (int fd);
typedef int (*fcntl_fun) (int fd, int cmd, ... /* arg */ );
typedef int (*ioctl_fun) (int fd, unsigned long request, ...);
typedef int (*getsockopt_fun) (int sockfd, int level, int optname, void *optval, socklen_t *optlen);
typedef int (*setsockopt_fun) (int sockfd, int level, int optname, const void *optval, socklen_t optlen);

// 原始系统调用函数指针声明
extern sleep_fun sleep_f;
extern usleep_fun usleep_f;
extern nanosleep_fun nanosleep_f;
extern socket_fun socket_f;
extern connect_fun connect_f;
extern accept_fun accept_f;
extern read_fun read_f;
extern readv_fun readv_f;
extern recv_fun recv_f;
extern recvfrom_fun recvfrom_f;
extern recvmsg_fun recvmsg_f;
extern write_fun write_f;
extern writev_fun writev_f;
extern send_fun send_f;
extern sendto_fun sendto_f;
extern sendmsg_fun sendmsg_f;
extern close_fun close_f;
extern fcntl_fun fcntl_f;
extern ioctl_fun ioctl_f;
extern getsockopt_fun getsockopt_f;
extern setsockopt_fun setsockopt_f;
```

## 3. API 接口说明

### 3.1 钩子控制接口

#### 3.1.1 is_hook_enable()

**功能**：检查钩子是否启用

**返回值**：钩子启用返回 `true`，否则返回 `false`

**使用示例**：
```cpp
if (mycoroutine::is_hook_enable()) {
    std::cout << "Hook is enabled" << std::endl;
} else {
    std::cout << "Hook is disabled" << std::endl;
}
```

#### 3.1.2 set_hook_enable(bool flag)

**功能**：设置钩子启用状态

**参数**：
- `flag`：启用标志，`true` 表示启用，`false` 表示禁用

**使用示例**：
```cpp
// 启用钩子
mycoroutine::set_hook_enable(true);

// 禁用钩子
mycoroutine::set_hook_enable(false);
```

### 3.2 系统调用钩子

系统调用钩子模块拦截并转换以下系统调用，保持与原始系统调用相同的函数签名和行为：

#### 3.2.1 睡眠函数

```cpp
// 睡眠函数钩子
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usce);
int nanosleep(const struct timespec* req, struct timespec* rem);
```

**功能**：将阻塞式睡眠转换为非阻塞的协程挂起

**说明**：
- 这些函数会挂起当前协程，而不是阻塞整个线程
- 其他协程可以在当前协程睡眠期间继续执行
- 返回值与原始函数相同，但在协程实现中总是返回 0

#### 3.2.2 网络函数

```cpp
// 网络函数钩子
int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

**功能**：处理网络相关的系统调用

**说明**：
- `socket()`：创建套接字并管理其上下文
- `connect()`：将阻塞式连接转换为非阻塞的协程挂起
- `accept()`：将阻塞式接受连接转换为非阻塞的协程挂起

#### 3.2.3 读取函数

```cpp
// 读取函数钩子
ssize_t read(int fd, void *buf, size_t count);
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
```

**功能**：将阻塞式读取转换为非阻塞的协程挂起

**说明**：
- 这些函数会尝试立即读取数据，如果数据不可用，会挂起当前协程
- 当数据可用时，协程会被唤醒并继续执行
- 返回值与原始函数相同

#### 3.2.4 写入函数

```cpp
// 写入函数钩子
ssize_t write(int fd, const void *buf, size_t count);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
```

**功能**：将阻塞式写入转换为非阻塞的协程挂起

**说明**：
- 这些函数会尝试立即写入数据，如果写入缓冲区已满，会挂起当前协程
- 当写入缓冲区可用时，协程会被唤醒并继续执行
- 返回值与原始函数相同

#### 3.2.5 文件描述符函数

```cpp
// 文件描述符函数钩子
int close(int fd);
```

**功能**：关闭文件描述符并清理相关资源

**说明**：
- 关闭文件描述符并清理相关的上下文信息
- 确保资源被正确释放

#### 3.2.6 文件控制函数

```cpp
// 文件控制函数钩子
int fcntl(int fd, int cmd, ... /* arg */ );
int ioctl(int fd, unsigned long request, ...);
```

**功能**：处理文件描述符控制操作

**说明**：
- 特别处理 `F_SETFL` 命令，用于设置非阻塞模式
- 特别处理 `FIONBIO` 请求，用于设置非阻塞 IO
- 其他命令会直接调用原始系统调用

#### 3.2.7 套接字选项函数

```cpp
// 套接字选项函数钩子
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```

**功能**：处理套接字选项的获取和设置

**说明**：
- 直接调用原始系统调用
- 确保套接字选项的正确性

## 4. 实现原理

### 4.1 钩子实现机制

系统调用钩子模块使用以下机制实现函数拦截和重定向：

1. **动态链接**：通过 LD_PRELOAD 环境变量或其他动态链接技术，将钩子库优先加载
2. **函数重定向**：使用与系统调用相同的函数名实现钩子函数，从而覆盖原始系统调用
3. **原始函数保存**：在钩子初始化时，保存原始系统调用的函数指针
4. **条件调用**：根据钩子启用状态，决定是调用钩子函数还是原始函数

### 4.2 非阻塞转换原理

系统调用钩子模块将阻塞式 IO 转换为非阻塞的协程挂起，主要过程如下：

1. **设置非阻塞**：确保文件描述符处于非阻塞模式
2. **尝试操作**：调用非阻塞版本的系统调用
3. **检查结果**：如果操作成功或遇到非阻塞错误（EAGAIN/EWOULDBLOCK），则继续执行；否则返回错误
4. **挂起协程**：如果遇到非阻塞错误，将当前协程挂起，并向 IO 管理器注册 IO 事件
5. **唤醒协程**：当 IO 事件就绪时，IO 管理器唤醒对应的协程
6. **恢复执行**：协程恢复执行后，重新尝试 IO 操作

### 4.3 睡眠函数实现

睡眠函数钩子的实现原理：

1. **获取当前协程**：获取当前正在执行的协程
2. **添加定时器**：向定时器管理器添加一个定时器，超时时间为指定的睡眠时长
3. **挂起协程**：挂起当前协程
4. **唤醒协程**：当定时器超时时，定时器管理器唤醒对应的协程
5. **恢复执行**：协程恢复执行

### 4.4 钩子启用机制

钩子启用机制使用线程局部存储（TLS）来存储钩子的启用状态，确保每个线程可以独立控制钩子的启用和禁用。

- `is_hook_enable()`：检查当前线程的钩子启用状态
- `set_hook_enable(bool flag)`：设置当前线程的钩子启用状态

## 5. 使用示例

### 5.1 基本使用

```cpp
#include <iostream>
#include <mycoroutine/hook.h>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

void test_hook() {
    std::cout << "Hook enabled: " << (is_hook_enable() ? "yes" : "no") << std::endl;
    
    // 启用钩子
    set_hook_enable(true);
    
    std::cout << "Hook enabled: " << (is_hook_enable() ? "yes" : "no") << std::endl;
    
    // 测试睡眠函数
    std::cout << "Before sleep" << std::endl;
    sleep(1); // 协程挂起，而不是阻塞线程
    std::cout << "After sleep" << std::endl;
    
    // 禁用钩子
    set_hook_enable(false);
    
    std::cout << "Hook enabled: " << (is_hook_enable() ? "yes" : "no") << std::endl;
}

int main() {
    IOManager iom(1, true, "HookTest");
    
    iom.scheduleLock(test_hook);
    
    iom.start();
    
    return 0;
}
```

### 5.2 网络 IO 示例

```cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mycoroutine/hook.h>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

void tcp_client() {
    // 启用钩子
    set_hook_enable(true);
    
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // 设置服务器地址
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // 连接服务器（非阻塞，协程挂起）
    if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(sockfd);
        return;
    }
    
    // 发送数据（非阻塞，协程挂起）
    const char* msg = "Hello, Server!";
    int n = send(sockfd, msg, strlen(msg), 0);
    if (n < 0) {
        std::cerr << "Failed to send data" << std::endl;
        close(sockfd);
        return;
    }
    
    // 接收数据（非阻塞，协程挂起）
    char buf[1024] = {0};
    n = recv(sockfd, buf, sizeof(buf), 0);
    if (n < 0) {
        std::cerr << "Failed to receive data" << std::endl;
        close(sockfd);
        return;
    }
    
    std::cout << "Received: " << buf << std::endl;
    
    // 关闭套接字
    close(sockfd);
}

int main() {
    IOManager iom(1, true, "NetworkTest");
    
    iom.scheduleLock(tcp_client);
    
    iom.start();
    
    return 0;
}
```

## 6. 性能优化

### 6.1 钩子开销优化

- 钩子函数尽量简短，减少额外开销
- 只有在必要时才调用复杂的逻辑
- 对于不需要转换的系统调用，直接调用原始函数

### 6.2 非阻塞设置优化

- 只在首次使用文件描述符时设置非阻塞模式
- 缓存文件描述符的非阻塞状态，避免重复设置

### 6.3 协程挂起优化

- 避免频繁的协程挂起和唤醒
- 合并多个相关的 IO 操作，减少协程切换

### 6.4 定时器优化

- 使用高效的定时器实现，减少定时器管理开销
- 对于短时间睡眠，考虑使用其他机制，如事件循环

## 7. 注意事项

### 7.1 钩子启用时机

- 钩子应该在程序启动时启用，或在创建协程之前启用
- 避免在协程执行过程中频繁切换钩子状态

### 7.2 文件描述符管理

- 钩子模块会管理文件描述符的上下文，包括非阻塞状态和超时设置
- 关闭文件描述符时，应该使用钩子包装后的 `close()` 函数，确保资源被正确释放

### 7.3 线程安全

- 钩子的启用状态是线程局部的，每个线程可以独立控制钩子的启用和禁用
- 钩子函数本身是线程安全的，可以在多个线程中同时调用

### 7.4 系统调用支持

- 钩子模块支持大多数常见的系统调用，但不是所有系统调用都被钩子覆盖
- 对于未被钩子覆盖的系统调用，会直接调用原始函数
- 不支持的系统调用可能会阻塞整个线程，而不是只挂起当前协程

### 7.5 错误处理

- 钩子函数会保持与原始系统调用相同的错误处理方式
- 错误码和返回值与原始系统调用一致
- 应该按照原始系统调用的方式处理错误

### 7.6 调试

- 在调试程序时，可以禁用钩子，使用原始系统调用，便于调试和分析
- 钩子可能会影响某些调试工具的正常工作

## 8. 总结

系统调用钩子模块是 mycoroutine 库的核心组件之一，提供了透明的非阻塞 IO 支持。该模块通过拦截和转换系统调用，将阻塞式 IO 转换为非阻塞的协程挂起，允许用户使用熟悉的阻塞式 API 编写异步程序，同时获得异步 IO 的性能优势。

系统调用钩子模块具有以下特点：

- 透明性：无需修改现有代码即可享受非阻塞 IO 带来的性能优势
- 高性能：最小化钩子带来的额外开销
- 可靠性：确保系统调用的正确性和一致性
- 灵活性：支持动态启用和禁用钩子
- 可扩展性：支持添加新的系统调用钩子

该模块的设计和实现为 mycoroutine 库的高效 IO 操作提供了基础，是构建高性能网络应用的重要组件。