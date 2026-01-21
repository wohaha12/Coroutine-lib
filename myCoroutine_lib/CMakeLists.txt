cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(mycoroutine VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -Werror -g)

# 头文件搜索路径
include_directories(include)

# 创建静态库
add_library(mycoroutine STATIC
    src/thread.cpp
    src/fiber.cpp
    src/scheduler.cpp
    src/timer.cpp
    src/iomanager.cpp
    src/hook.cpp
    src/fd_manager.cpp
    src/utils.cpp
)

# 链接依赖
target_link_libraries(mycoroutine pthread)

# 添加子目录
add_subdirectory(examples)
