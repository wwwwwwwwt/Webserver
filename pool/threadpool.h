/*
 * @Author: zzzzztw
 * @Date: 2023-02-20 12:53:39
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-20 14:01:20
 * @FilePath: /Webserver/pool/threadpool.h
 */
#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPool {
public:
    /*
        web框架作为i/o密集型，线程数 = ncpu*（1+阻塞时间/计算时间）一般取2Ncpu
        计算密集型 ncpu +1
    */
    explicit ThreadPool(size_t threadcount = 8):pool_(std::make_shared<Pool>()){
        assert(threadcount > 0);

        for(size_t i = 0;i < threadcount; i++){
            std::thread([&](){
                std::unique_lock<std::mutex>locker(pool_->mtx_);    
                while(true){
                    if(!pool_->tasks_.empty()){
                        auto t = std::move(pool_->tasks_.front());
                        pool_->tasks_.pop();
                        locker.unlock();//减小粒度 解锁执行任务，其他线程可以从队列继续取任务
                        t();
                        locker.lock();
                    }
                    else if (pool_->isclose)break;
                    else pool_->cond_.wait(locker);
                }     
            }).detach();
        }
        }
        
        ThreadPool() = default;

        ThreadPool(ThreadPool &&) = default;

        template<typename T>
        void AddTask(T && task){
            {
                //std::unique_lock<std:: mutex>locker(pool_->mtx_);
                std::lock_guard<std::mutex>locker(pool_->mtx_);
                pool_->tasks_.emplace(std::forward<T>(task));
            }
            pool_->cond_.notify_one();
        }
    

private:
    struct Pool{
        std::mutex mtx_;
        std::condition_variable cond_;
        bool isclose;
        std::queue<std::function<void()>>tasks_;

    };

    std::shared_ptr<Pool> pool_;
};

