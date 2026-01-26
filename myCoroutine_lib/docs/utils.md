# 工具库模块 (Utils)

## 1. 模块概述

工具库模块是 mycoroutine 库的辅助组件，提供了通用的工具函数和组件，为其他模块提供基础支持。目前该模块主要包含日志系统，用于记录和管理日志信息，支持不同级别的日志输出和灵活的配置。

### 1.1 主要功能
- 支持多种日志级别（DEBUG、INFO、WARN、ERROR、FATAL）
- 线程安全的日志输出
- 包含文件名和行号的日志信息
- 支持格式化字符串日志
- 单例模式的日志器实现
- 灵活的日志级别配置

### 1.2 设计目标
- 简单易用：提供简洁的API接口
- 高性能：最小化日志输出的性能开销
- 线程安全：支持多线程并发日志输出
- 灵活配置：支持动态调整日志级别
- 清晰的日志格式：包含必要的上下文信息

## 2. 核心设计

### 2.1 日志级别

工具库模块定义了以下日志级别：

| 级别 | 描述 | 数值 |
|-----|-----|-----|
| DEBUG | 调试信息，用于开发和调试 | 0 |
| INFO | 普通信息，用于记录程序运行状态 | 1 |
| WARN | 警告信息，用于记录可能的问题 | 2 |
| ERROR | 错误信息，用于记录错误情况 | 3 |
| FATAL | 致命错误，用于记录导致程序终止的严重错误 | 4 |
| UNKNOWN | 未知级别 | 5 |

### 2.2 类结构设计

```cpp
// 日志级别枚举
enum class LogLevel {
    DEBUG   = 0,
    INFO    = 1,
    WARN    = 2,
    ERROR   = 3,
    FATAL   = 4,
    UNKNOWN = 5
};

// 日志工具类
class Logger {
private:
    Logger();
    ~Logger();
    
    const char* levelToString(LogLevel level) const;
    std::string getTimeString() const;
    
private:
    LogLevel m_level;       // 当前日志级别
    std::mutex m_mutex;     // 日志输出互斥锁
    
public:
    static Logger& GetInstance();
    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    void log(LogLevel level, const char* file, int line, const char* format, ...);
};

// 日志宏定义
#define MYCOROUTINE_LOG(level, ...) \
    do { \
        mycoroutine::Logger& logger = mycoroutine::Logger::GetInstance(); \
        if (logger.getLevel() <= mycoroutine::LogLevel::level) { \
            logger.log(mycoroutine::LogLevel::level, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while (0)

// 各级别日志宏
#define MYCOROUTINE_LOG_DEBUG(...)  MYCOROUTINE_LOG(DEBUG, __VA_ARGS__)
#define MYCOROUTINE_LOG_INFO(...)   MYCOROUTINE_LOG(INFO, __VA_ARGS__)
#define MYCOROUTINE_LOG_WARN(...)   MYCOROUTINE_LOG(WARN, __VA_ARGS__)
#define MYCOROUTINE_LOG_ERROR(...)  MYCOROUTINE_LOG(ERROR, __VA_ARGS__)
#define MYCOROUTINE_LOG_FATAL(...)  MYCOROUTINE_LOG(FATAL, __VA_ARGS__)
```

### 2.3 日志格式

日志系统输出的日志格式如下：

```
[时间] [日志级别] [文件名:行号] 日志内容
```

例如：
```
[2023-01-01 12:00:00] [INFO] [main.cpp:42] Program started
[2023-01-01 12:00:01] [ERROR] [fiber.cpp:123] Fiber creation failed
```

## 3. API 接口说明

### 3.1 Logger 类接口

#### 3.1.1 GetInstance()

**功能**：获取日志器单例实例

**返回值**：日志器单例引用

**使用示例**：
```cpp
auto& logger = mycoroutine::Logger::GetInstance();
```

#### 3.1.2 setLevel(LogLevel level)

**功能**：设置日志级别

**参数**：
- `level`：日志级别

**使用示例**：
```cpp
// 设置日志级别为INFO
logger.setLevel(mycoroutine::LogLevel::INFO);
```

#### 3.1.3 getLevel()

**功能**：获取当前日志级别

**返回值**：当前日志级别

**使用示例**：
```cpp
auto level = logger.getLevel();
if (level <= mycoroutine::LogLevel::DEBUG) {
    // 调试模式下的额外处理
}
```

#### 3.1.4 log(LogLevel level, const char* file, int line, const char* format, ...)

**功能**：记录日志

**参数**：
- `level`：日志级别
- `file`：文件名
- `line`：行号
- `format`：格式化字符串
- `...`：可变参数

**使用示例**：
```cpp
// 直接调用log方法（不推荐，建议使用日志宏）
logger.log(mycoroutine::LogLevel::INFO, __FILE__, __LINE__, "Program started");
```

### 3.2 日志宏

工具库模块提供了以下日志宏，用于简化日志记录：

#### 3.2.1 MYCOROUTINE_LOG(level, ...)

**功能**：通用日志宏

**参数**：
- `level`：日志级别（DEBUG、INFO、WARN、ERROR、FATAL）
- `...`：格式化字符串和可变参数

**使用示例**：
```cpp
MYCOROUTINE_LOG(INFO, "Program started");
MYCOROUTINE_LOG(DEBUG, "Value: %d", 42);
```

#### 3.2.2 级别日志宏

```cpp
// 调试日志
MYCOROUTINE_LOG_DEBUG("Debug message: %s", message);

// 普通信息日志
MYCOROUTINE_LOG_INFO("Info message");

// 警告日志
MYCOROUTINE_LOG_WARN("Warning message");

// 错误日志
MYCOROUTINE_LOG_ERROR("Error message: %d", errno);

// 致命错误日志
MYCOROUTINE_LOG_FATAL("Fatal error");
```

## 4. 实现原理

### 4.1 单例模式实现

日志器采用单例模式实现，确保全局只有一个日志器实例：

```cpp
Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}
```

这种实现方式利用了 C++11 静态局部变量的线程安全性，确保在多线程环境下也能正确创建单例实例。

### 4.2 线程安全实现

日志系统使用互斥锁确保线程安全：

```cpp
void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
    if (level < m_level) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    // 日志输出逻辑
}
```

在 `log` 方法中，使用 `std::lock_guard` 自动管理互斥锁，确保同一时间只有一个线程可以输出日志，避免日志信息混乱。

### 4.3 日志级别过滤

日志系统根据当前设置的日志级别过滤日志：

```cpp
#define MYCOROUTINE_LOG(level, ...) \
    do { \
        mycoroutine::Logger& logger = mycoroutine::Logger::GetInstance(); \
        if (logger.getLevel() <= mycoroutine::LogLevel::level) { \
            logger.log(mycoroutine::LogLevel::level, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while (0)
```

日志宏在调用 `log` 方法前，会检查当前日志级别是否大于等于要输出的日志级别，只有满足条件的日志才会被输出，减少不必要的性能开销。

### 4.4 日志格式生成

日志系统生成的日志包含以下信息：

1. **时间**：当前系统时间，格式为 `YYYY-MM-DD HH:MM:SS`
2. **日志级别**：日志的级别字符串，如 `[INFO]`、`[ERROR]`
3. **文件名和行号**：日志产生的位置，如 `[main.cpp:42]`
4. **日志内容**：用户提供的日志信息

### 4.5 格式化字符串处理

日志系统使用 `vsnprintf` 处理格式化字符串和可变参数，确保日志内容的正确生成：

```cpp
void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
    // ...
    va_list args;
    va_start(args, format);
    
    char buffer[1024] = {0};
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    // 输出日志
    // ...
}
```

## 5. 使用示例

### 5.1 基本使用

```cpp
#include <mycoroutine/utils.h>

using namespace mycoroutine;

int main() {
    // 获取日志器实例
    auto& logger = Logger::GetInstance();
    
    // 设置日志级别为DEBUG
    logger.setLevel(LogLevel::DEBUG);
    
    // 使用日志宏记录日志
    MYCOROUTINE_LOG_DEBUG("Program started");
    
    int value = 42;
    MYCOROUTINE_LOG_INFO("Value: %d", value);
    
    if (value > 50) {
        MYCOROUTINE_LOG_WARN("Value is too large");
    } else {
        MYCOROUTINE_LOG_DEBUG("Value is within range");
    }
    
    try {
        // 模拟异常
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        MYCOROUTINE_LOG_ERROR("Exception: %s", e.what());
    }
    
    MYCOROUTINE_LOG_FATAL("Program terminated");
    
    return 0;
}
```

### 5.2 在类中使用

```cpp
#include <mycoroutine/utils.h>

class MyClass {
public:
    void doSomething() {
        MYCOROUTINE_LOG_INFO("MyClass::doSomething called");
        
        // 执行一些操作
        int result = operation();
        
        MYCOROUTINE_LOG_DEBUG("Operation result: %d", result);
        
        if (result < 0) {
            MYCOROUTINE_LOG_ERROR("Operation failed with result: %d", result);
        }
    }
    
private:
    int operation() {
        MYCOROUTINE_LOG_DEBUG("MyClass::operation called");
        // 模拟操作
        return 42;
    }
};

int main() {
    MyClass obj;
    obj.doSomething();
    return 0;
}
```

### 5.3 动态调整日志级别

```cpp
#include <mycoroutine/utils.h>

using namespace mycoroutine;

int main() {
    auto& logger = Logger::GetInstance();
    
    // 设置初始日志级别为INFO
    logger.setLevel(LogLevel::INFO);
    
    MYCOROUTINE_LOG_DEBUG("This message will not be displayed");
    MYCOROUTINE_LOG_INFO("This message will be displayed");
    
    // 动态调整日志级别为DEBUG
    logger.setLevel(LogLevel::DEBUG);
    
    MYCOROUTINE_LOG_DEBUG("Now this message will be displayed");
    
    return 0;
}
```

## 6. 性能优化

### 6.1 日志级别过滤

- 日志宏在调用 `log` 方法前，会检查日志级别，避免不必要的函数调用和格式化操作
- 对于频繁调用的日志，可以考虑使用条件编译或运行时检查，减少性能开销

### 6.2 格式化字符串处理

- 使用 `vsnprintf` 处理格式化字符串，确保线程安全和正确的内存管理
- 限制日志缓冲区大小，避免过大的内存分配

### 6.3 互斥锁优化

- 使用 `std::lock_guard` 自动管理互斥锁，避免死锁
- 日志输出操作尽量简洁，减少持有互斥锁的时间

### 6.4 异步日志（未来扩展）

当前版本的日志系统是同步的，未来可以考虑添加异步日志支持：
- 将日志消息放入队列，由专门的日志线程处理
- 减少日志输出对主线程的阻塞
- 支持日志文件滚动和压缩

## 7. 注意事项

### 7.1 日志级别使用

- 合理使用日志级别，避免过度记录日志
- DEBUG级别用于开发和调试，生产环境中应使用较高的日志级别
- ERROR和FATAL级别用于严重错误，应及时处理

### 7.2 日志内容

- 日志内容应简洁明了，包含必要的上下文信息
- 避免记录敏感信息（如密码、密钥等）
- 日志格式应一致，便于日志分析和处理

### 7.3 性能考虑

- 避免在性能关键路径上频繁记录日志
- 对于频繁调用的日志，可以考虑使用条件编译
- 生产环境中应适当提高日志级别，减少日志输出量

### 7.4 线程安全

- 日志系统是线程安全的，可以在多线程环境中使用
- 但应避免在日志回调中调用其他可能导致死锁的函数

### 7.5 文件名和行号

- 日志宏会自动包含文件名和行号，便于定位问题
- 直接调用 `log` 方法时，需要手动提供文件名和行号

## 8. 总结

工具库模块是 mycoroutine 库的辅助组件，目前主要包含日志系统。该日志系统具有以下特点：

- 简单易用的API接口
- 支持多种日志级别
- 线程安全的日志输出
- 包含文件名和行号的日志信息
- 支持格式化字符串日志
- 单例模式的日志器实现
- 灵活的日志级别配置

日志系统为其他模块提供了基础的日志记录功能，便于调试和监控程序运行状态。未来可以考虑扩展工具库模块，添加更多通用工具函数，如字符串处理、时间管理、文件操作等，进一步提高库的实用性和灵活性。