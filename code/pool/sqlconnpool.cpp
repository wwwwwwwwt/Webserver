/*
 * @Author: zzzzztw
 * @Date: 2023-02-23 15:12:04
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 13:07:06
 * @FilePath: /Webserver/code/pool/sqlconnpool.cpp
 */
#include "./sqlconnpool.h"
#include <iostream>
using namespace std;

SqlConnPool::SqlConnPool(){
    UseCount_ = 0;
    FreeCount_ = 0;
}
SqlConnPool::~SqlConnPool(){
    ClosePool();
}

SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host,int port,const char* user,const char* pwd,const char* dbName,int connSize){
    assert(connSize > 0);
    for(int i = 0;i < connSize;i++){//默认十个连接
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);//传入空指针获得新的MYSQL对象句柄
        if(!sql){
            //log 初始化错误可能是内存不足 返回null
            assert(!sql);
        }
        sql = mysql_real_connect(sql,host,user,pwd,dbName,port,nullptr,0);

        if(!sql){
            //log

        }
        conQue_.push(sql);
    }
    MaxConn_ = connSize;
    sem_init(&semId_,0,MaxConn_);//初始化信号量，wait-1，减到0时阻塞， 释放时调用sem_post 信号量+1
}

MYSQL* SqlConnPool::GetConn(){
    MYSQL* sql = nullptr;
    if(conQue_.empty()){
        //log 繁忙
        return nullptr;
    }
    {
        lock_guard<mutex>locker(mtx_);
        sql = conQue_.front();
        conQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* conn){
    assert(conn);
    lock_guard<mutex>locker(mtx_);
    conQue_.push(conn);
    sem_post(&semId_);
}


void SqlConnPool::ClosePool(){
    
    lock_guard<mutex>locker(mtx_);
    while(!conQue_.empty()){
        auto sql = conQue_.front();
        conQue_.pop();
        if(sql !=nullptr){
            mysql_close(sql);
        }
        
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount(){
    lock_guard<mutex>locker(mtx_);
    return conQue_.size();
}
