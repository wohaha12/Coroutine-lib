// 包含必要的头文件
#include "ioscheduler.h"  // 引入IO管理器头文件
#include <iostream>        // 标准输入输出流
#include <sys/socket.h>    // 套接字相关函数
#include <netinet/in.h>    // 互联网地址族
#include <arpa/inet.h>     // 地址转换函数
#include <fcntl.h>         // 文件控制函数
#include <unistd.h>        // Unix标准函数
#include <sys/epoll.h>     // epoll IO多路复用
#include <cstring>         // C风格字符串处理
#include <cerrno>          // 错误码定义

// 使用sylar命名空间
using namespace sylar;

// 接收数据的缓冲区
char recv_data[4096];

// HTTP GET请求数据，用于发送给服务器
const char data[] = "GET / HTTP/1.0\r\n\r\n"; 

// 全局套接字描述符，用于网络通信
int sock;

/**
 * @brief 读取数据回调函数
 * 当套接字可读时，此函数被调用以接收服务器响应数据
 */
void func()
{
    // 从套接字接收数据到缓冲区
    recv(sock, recv_data, 4096, 0);
    // 输出接收到的数据到控制台
    std::cout << recv_data << std::endl << std::endl;
}

/**
 * @brief 发送数据回调函数
 * 当套接字可写时，此函数被调用以发送HTTP GET请求
 */
void func2()
{
    // 发送HTTP GET请求数据到服务器
    send(sock, data, sizeof(data), 0);
}

/**
 * @brief 主函数
 * 程序入口点，创建IO管理器并执行非阻塞网络通信示例
 */
int main(int argc, char const *argv[])
{
    // 创建IO管理器实例，线程池大小为2
    IOManager manager(2);

    // 创建TCP套接字（IPv4，流式传输，默认协议）
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 配置服务器地址信息
    sockaddr_in server;
    server.sin_family = AF_INET;              // IPv4协议
    server.sin_port = htons(80);              // HTTP标准端口（转换为网络字节序）
    server.sin_addr.s_addr = inet_addr("103.235.46.96");  // 服务器IP地址

    // 设置套接字为非阻塞模式
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // 非阻塞连接到服务器
    // 注意：非阻塞连接通常会立即返回，需要通过事件机制处理连接完成
    connect(sock, (struct sockaddr *)&server, sizeof(server));
   
    // 注册IO事件到管理器
    // 当套接字可写时，调用func2发送HTTP请求
    manager.addEvent(sock, IOManager::WRITE, &func2);
    // 当套接字可读时，调用func接收HTTP响应
    manager.addEvent(sock, IOManager::READ, &func);

    // 输出提示信息
    std::cout << "event has been posted\n\n";
    
    // 注意：此处直接返回0会导致程序退出
    // IO管理器的工作线程需要足够时间处理已注册的事件
    sleep(10);  // 等待10秒，确保IO操作完成
    
    return 0;
}