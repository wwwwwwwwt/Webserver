<!--
 * @Author: zzzzztw
 * @Date: 2023-03-15 18:58:59
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-15 19:31:11
 * @FilePath: /Webserver/README.md
-->
# 一个基于c++11的轻量级Web并发服务器开发项目

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则表达式与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。
* 基于小根堆与多级时间轮（to do）共同实现了定时器的通用接口，关闭超时的非活动连接，使得服务器可以实现多种场景的定时功能；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；


## 项目启动
需要先配置好对应的数据库
```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');
```

```bash
make
./bin/server
```

## 压力测试
```
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
```
* 测试环境Ubuntu22.04 2核4线程下实现了 6000+的QPS。 

## TO do
* 多级时间轮的接口
* 修改日志模块