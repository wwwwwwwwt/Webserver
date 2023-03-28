/*
 * @Author: zzzzztw
 * @Date: 2023-03-09 14:00:30
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-28 18:37:35
 * @FilePath: /Webserver/code/log/safequeue.h
 */

#ifndef  SAFEQUEUE_H
#define SAFEQUEUE_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <cassert>
#include <iostream>
template <typename T>
class Safequeue
{
public:
    explicit Safequeue(size_t maxCapacity = 1000);

    ~Safequeue();

    Safequeue(Safequeue &&s) = default;

    Safequeue &operator=(Safequeue &&s) = default;

    void close();

    void enqueue(const T& t);

    void enqueue(T&& t);

    bool dequeue(T& t);

    void flush();

    bool empty();

    bool full();
    
    size_t size();

    size_t capacity();

private:
    size_t capacity_;

    std::mutex mtx_;

    std::queue<T>que_;

    std::condition_variable condProducer_;

    std::condition_variable condCumsumer_;

    bool isClose_;
};

template<typename T>
Safequeue<T>::Safequeue(size_t maxCapacity):capacity_(maxCapacity)
{
    assert(maxCapacity > 0);
    isClose_ = false;
}

template<typename T>
Safequeue<T>::~Safequeue()
{
    close();
}

template<typename T>
void Safequeue<T>::close()
{
    {
        std::lock_guard<std::mutex>locker(mtx_);
        while(!que_.empty())que_.pop();
        isClose_ = true;
    }
    condCumsumer_.notify_all();
    condProducer_.notify_all();
}


template<typename T>
void Safequeue<T>::enqueue(const T& t)
{
    std::unique_lock<std::mutex>locker(mtx_);
    condProducer_.wait(locker,[&]{
        if(isClose_)return true;
        return que_.size() < capacity_;
    });
    if(isClose_)return;
    que_.emplace(t);
    condCumsumer_.notify_all();
}

template<typename T>
void Safequeue<T>::enqueue(T&& t)
{
    std::unique_lock<std::mutex>locker(mtx_);
    condProducer_.wait(locker,[&]{
        if(isClose_)return true;
        return que_.size() < capacity_;
    });
    if(isClose_)return;
    que_.emplace(t);
    condCumsumer_.notify_all();
}

template<typename T>
bool Safequeue<T>::dequeue(T& t)
{
    std::unique_lock<std::mutex>locker(mtx_);
    condCumsumer_.wait(locker,[&]{
        if(isClose_)return true;
        return !que_.empty();
    });
    if(isClose_)return false;
    t = std::move(que_.front());
    que_.pop();
    condProducer_.notify_all();

    return true;
}

template<typename T>
void Safequeue<T>::flush()
{
    std::lock_guard<std::mutex>locker(mtx_);
    condCumsumer_.notify_one();
}

template<typename T>
bool Safequeue<T>::full()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return que_.size() >= capacity_;
}

template<typename T>
bool Safequeue<T>::empty()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return que_.empty();
}

template<typename T>
size_t Safequeue<T>::size()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return que_.size();
}

template<typename T>
size_t Safequeue<T>::capacity()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return capacity_;
}



#endif