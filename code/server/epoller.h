/*
 * @Author: zzzzztw
 * @Date: 2023-02-20 14:41:26
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-21 16:14:26
 * @FilePath: /Webserver/server/epoller.h
 */

#ifndef EPOLLER_H
#define EPOLLER_H


#include <sys/epoll.h> // epoll_ctl()
#include <fcntl.h> // fcntl()
#include <unistd.h> // close()
#include <assert.h>
#include <vector>
#include <errno.h> 

class Epoller{
    
public:

    explicit Epoller(int MaxEvent = 1024);

    bool AddFd(int fd, uint32_t events);

    bool ModFD(int fd, uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs = -1); // 定时器

    int GetEventFd(size_t i)const;

    uint32_t GetEvents(size_t i)const;

    ~Epoller();


private:
    int epollFd_; // epoll_create()创建一个epoll对象，返回epollfd
    
    std::vector<struct epoll_event> events_; //检测到事件集合
};

#endif // EPOLLER_H