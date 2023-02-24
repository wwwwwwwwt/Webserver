/*
 * @Author: zzzzztw
 * @Date: 2023-02-23 15:11:51
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-23 19:09:50
 * @FilePath: /Webserver/pool/sqlconnpool.h
 */
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <cassert>
#include <semaphore.h>

class SqlConnPool{
public:
    static SqlConnPool *Instance(); //单例模式，私有化构造函数，函数内静态实例化对象并返回引用

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
                const char* user,const char* pwd,
                const char* dbName,int connSize = 10);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();
    SqlConnPool(const SqlConnPool & s) = delete;
    SqlConnPool& operator=(const SqlConnPool& sql) = delete;
    int MaxConn_;
    int UseCount_;
    int FreeCount_;

    std::queue<MYSQL *>conQue_;
    std::mutex mtx_;
    sem_t semId_;

};

#endif
