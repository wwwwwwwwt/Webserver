/*
 * @Author: zzzzztw
 * @Date: 2023-02-20 15:13:23
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-21 14:25:33
 * @FilePath: /Webserver/server/epoller.cpp
 */
#include "./epoller.h"

Epoller::Epoller(int MaxEvent):epollFd_(epoll_create(1024)),events_(MaxEvent){
    assert(epollFd_ >=0 && events_.size()>0);
}

bool Epoller::AddFd(int fd, uint32_t events){
    if(fd < 0)return false;

    epoll_event ev; 
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd, &ev) == 0;
}

bool Epoller::ModFD(int fd,uint32_t events){
    if(fd < 0)return false;

    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ev) == 0;

}

bool Epoller::DelFd(int fd){
    if(fd < 0) return false;
    epoll_event ev;
    return epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd, &ev);
}

int Epoller::Wait(int timeoutMs){
    return epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeoutMs);
}

int Epoller::GetEventFd(size_t i)const {
    assert(i<events_.size() && i>=0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i)const {
    assert(i<events_.size() && i>=0 );
    return events_[i].events;
}

Epoller::~Epoller(){
    close(epollFd_);
}
