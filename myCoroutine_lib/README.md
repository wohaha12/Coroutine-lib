# mycoroutine - C++17 协程库

## 项目概述

mycoroutine 是一个基于 C++17 标准开发的高性能协程库，提供了完整的协程实现、调度器、IO 多路复用以及系统调用钩子机制。该库旨在简化异步编程模型，提高程序的并发处理能力和资源利用率，同时保持良好的易用性和可扩展性。

## 为什么要做协程库？

1. **差异化优势**：相比传统的 WebServer 项目，协程库具有更高的技术含量和差异化优势
2. **实用价值**：协程作为强大的并发编程技术，在大量 I/O 操作或并发任务场景下能显著提高性能和可维护性
3. **易于集成**：协程库作为独立组件，可以方便地应用到其他项目中，为传统项目增添新意
4. **知识提升**：深入理解协程技术，增加知识深度和广度，提高面试通过率

## 核心功能亮点

- ✅ **高性能协程**：基于 ucontext 实现的有栈协程，切换开销低
- ✅ **强大的调度器**：支持多线程调度和负载均衡
- ✅ **高效 IO 管理**：基于 epoll 的 IO 多路复用，支持事件循环和定时器
- ✅ **透明非阻塞**：系统调用钩子机制，自动将阻塞 IO 转换为非阻塞协程挂起
- ✅ **灵活的定时器**：支持一次性和周期性定时器，精度可达毫秒级
- ✅ **完善的日志系统**：支持多种日志级别，线程安全的日志输出

## 快速开始

### 环境要求

- **编译器**：GCC 7+ 或 Clang 6+
- **C++标准**：C++17
- **构建系统**：CMake 3.15+
- **操作系统**：Linux

### 编译安装

```bash
# 克隆仓库
git clone https://github.com/wohaha12/Coroutine-lib.git
cd Coroutine-lib/myCoroutine_lib

# 构建
mkdir build && cd build
cmake ..
make

# 安装（可选）
sudo make install
```

### 简单示例

```cpp
#include <iostream>
#include <mycoroutine/fiber.h>
#include <mycoroutine/iomanager.h>

using namespace mycoroutine;

void test_fiber() {
    std::cout << "Fiber start" << std::endl;
    
    // 让出执行权
    Fiber::GetThis()->yield();
    
    std::cout << "Fiber resume" << std::endl;
}

int main() {
    // 创建IO管理器，使用4个线程
    IOManager iom(3, true, "TestIOManager");
    
    // 调度协程
    iom.scheduleLock([]() {
        test_fiber();
    });
    
    std::cout << "Main start" << std::endl;
    
    // 启动IO管理器
    iom.start();
    
    return 0;
}
```

## 模块导航

mycoroutine 库采用模块化设计，各模块职责清晰，低耦合高内聚。以下是核心模块的详细文档：

| 模块名称 | 主要职责 | 详细文档 |
|---------|---------|--------|
| **Fiber** | 协程核心实现 | [fiber.md](docs/fiber.md) |
| **Scheduler** | 协程调度器 | [scheduler.md](docs/scheduler.md) |
| **IOManager** | IO事件管理 | [iomanager.md](docs/iomanager.md) |
| **Timer** | 定时器管理 | [timer.md](docs/timer.md) |
| **Thread** | 线程管理 | [thread.md](docs/thread.md) |
| **FDManager** | 文件描述符管理 | [fd_manager.md](docs/fd_manager.md) |
| **Hook** | 系统调用钩子 | [hook.md](docs/hook.md) |
| **Utils** | 工具库 | [utils.md](docs/utils.md) |
| **架构概述** | 整体设计 | [architecture_overview.md](docs/architecture_overview.md) |

## 构建和测试

### 编译选项

- `-DCMAKE_BUILD_TYPE=Debug`：调试模式，包含调试信息
- `-DCMAKE_BUILD_TYPE=Release`：发布模式，优化编译
- `-DBUILD_EXAMPLES=ON`：构建示例程序（默认ON）
- `-DBUILD_TESTS=ON`：构建测试程序

### 运行示例

```bash
# 在build目录下
./examples/coroutine_http_server
```

### 运行测试

```bash
# 在build目录下
./tests/test_fiber
./tests/test_scheduler
./tests/test_iomanager
```

## 项目结构

```
myCoroutine_lib/
├── CMakeLists.txt          # 主CMake文件
├── README.md               # 项目说明文档
├── docs/                   # 详细文档目录
│   ├── architecture_overview.md
│   ├── fiber.md
│   ├── scheduler.md
│   ├── iomanager.md
│   ├── timer.md
│   ├── thread.md
│   ├── fd_manager.md
│   ├── hook.md
│   └── utils.md
├── examples/               # 示例程序
│   ├── CMakeLists.txt
│   └── coroutine_http_server.cpp
├── include/                # 头文件目录
│   └── mycoroutine/        # 库头文件
│       ├── fd_manager.h
│       ├── fiber.h
│       ├── hook.h
│       ├── iomanager.h
│       ├── scheduler.h
│       ├── thread.h
│       ├── timer.h
│       └── utils.h
├── src/                    # 源文件目录
│   ├── fd_manager.cpp
│   ├── fiber.cpp
│   ├── hook.cpp
│   ├── iomanager.cpp
│   ├── scheduler.cpp
│   ├── thread.cpp
│   ├── timer.cpp
│   └── utils.cpp
└── tests/                  # 测试程序
    ├── epoll/              # epoll测试
    └── libevent/           # libevent测试
```

