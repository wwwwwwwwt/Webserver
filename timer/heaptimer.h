/*
 * @Author: zzzzztw
 * @Date: 2023-03-04 14:15:30
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-04 17:14:14
 * @FilePath: /Webserver/timer/heaptimer.h
 */

#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include <queue>
#include <time.h>
#include <chrono>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>

using Ms = std::chrono::milliseconds;
using TimeStamp = std::chrono::high_resolution_clock::time_point;
using Callback = std::function<void()>;

struct Node{
    int id_;
    TimeStamp tstp_;
    Callback cb;
    bool operator<(const Node& t){
        return tstp_ < t.tstp_;
    };
};

class HeapTimer{
public:
    HeapTimer(){ heap_.reserve(64); }
    ~HeapTimer(){ clear(); };
    void add(int id, int timeout, const Callback& cb);
    void adjust(int id, int timeout);
    void tick();
    void pop();
    void clear();
    int GetNextTick();
private:
    void del_(size_t i);
    void up(size_t i);
    void down(size_t i);
    void swapNode(size_t i, size_t j);

    std::vector<Node>heap_;
    std::unordered_map<int,size_t>mp_; // 记录每个id，在堆中的下标映射 
};

#endif