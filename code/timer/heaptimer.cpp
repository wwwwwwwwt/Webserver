/*
 * @Author: zzzzztw
 * @Date: 2023-03-04 14:33:44
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-04 17:00:30
 * @FilePath: /Webserver/timer/heaptimer.cpp
 */
#include "./heaptimer.h"

using namespace std;

void HeapTimer::swapNode(size_t i, size_t j){
    assert(i >=0 && i < heap_.size()&&j >= 0 && j < heap_.size());
    swap(heap_[i],heap_[j]);

    mp_[heap_[i].id_] = i;
    mp_[heap_[j].id_] = j;
}

void HeapTimer::up(size_t i){
    assert(i >= 0 && i < heap_.size());
    while( (i-1)/2 &&heap_[i] < heap_[(i-1)/2])
    {
        swapNode(i, (i-1)/2);
        i>>=1;
    }
}

void HeapTimer::down(size_t i){

    assert(i >=0 && i< heap_.size());
    size_t t = i;
    size_t left = i * 2 +1;
    size_t right = i* 2 + 2;
    if(left < heap_.size() && heap_[i*2+1] < heap_[t]) t = left;
    if(right < heap_.size() && heap_[i*2+2] < heap_[t] ) t =right;
    if(i != t){
        swapNode(i,t);
        down(t);
    }

}


void HeapTimer::del_(size_t i){
    assert(!heap_.empty() && i >=0 && i < heap_.size());

    size_t eend = heap_.size() - 1;

    if(i < eend){
        swapNode(i,eend);
        mp_.erase(heap_.back().id_);
        heap_.pop_back();
        up(i);
        down(i);
    }
    else{
        mp_.erase(heap_.back().id_);
        heap_.pop_back();
    }
}

void HeapTimer::add(int id, int timeout, const Callback& cb) {
    assert(id >= 0);
    size_t i;
    if(mp_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        mp_[id] = i;
        heap_.push_back({id, chrono::high_resolution_clock::now() + Ms(timeout), cb});
        up(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = mp_[id];
        heap_[i].tstp_ = chrono::high_resolution_clock::now() + Ms(timeout);
        heap_[i].cb = cb;
        down(i);
        up(i);
    }
}
void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && mp_.count(id) > 0);
    heap_[mp_[id]].tstp_ = chrono::high_resolution_clock::now() + Ms(timeout);;
    down(mp_[id]);
}

void HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        Node node = heap_.front();
        if(chrono::duration_cast<Ms>(node.tstp_ - chrono::high_resolution_clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    mp_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<Ms>(heap_.front().tstp_ - chrono::high_resolution_clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}
