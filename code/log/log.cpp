/*
 * @Author: zzzzztw
 * @Date: 2023-03-09 21:48:40
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-12 13:35:02
 * @FilePath: /Webserver/code/log/log.cpp
 */

#include "./log.h"

using namespace std;

Log::Log(){
    lineCount_ = 0;
    toDay_ = 0;
    isOpen_ = false;
    fd = nullptr;
    queue_ = nullptr;
    level_ = 0;
    isAsync_ = false;
    
}

Log* Log::Instance(){
    static Log instance_;
    return &instance_;
}


Log::~Log()
{
    if(writeThread_ && writeThread_->joinable()){
        while(!queue_->empty()){
            queue_->flush();
        }
        queue_->close();
        writeThread_->join();
    }
    if(fd){
        lock_guard<mutex>locker(mtx_);
        flush();
        fclose(fd);
    }
}
void Log::init(int  level = 1, const char* path, const char* suffix, int safeQueueCap){
    level_ = level;
    lineCount_ = 0;
    path_ = path;
    suffix_ = suffix;
    isOpen_ = true;

    if(safeQueueCap > 0){
        isAsync_ = true;
        if(!queue_){
            unique_ptr<Safequeue<string>>newQue(new Safequeue<string>());
            queue_ = move(newQue);
        }
    }else{
        isAsync_ = false;
    }

    lineCount_ = 0; // 当前写了几行

    time_t timer = time(nullptr); // 获取时间戳
    struct tm *sysTime = localtime(&timer);//通过时间戳获取本地时间
    
    struct tm t = *sysTime;
    
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", //输出格式 202_11—_28.log
        path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,suffix_);

    toDay_ = t.tm_mday;
    {
        lock_guard<mutex>locker(mtx_);
        buff_.RetrieveAll();
        if(fd){
            flush();
            fclose(fd);
        }
        //以追加模式打开文件
        fd = fopen(fileName, "a");
        if(fd == nullptr){
            mkdir(path_, 0777);
            fd = fopen(fileName, "a");
        }
        assert(fd != nullptr);
    }
}

void Log::write(int level, const char* format,...)
{
    struct timeval now = {0,0};
    //gettimeofday(&now,nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);

}


void Log::flush(){
    if(isAsync_){
        queue_->flush();
    }
    fflush(fd);
}

int  Log::GetLevel()
{
    lock_guard<mutex>locker(mtx_);
    return level_;
}

void Log::SetLevel(int level)
{
    lock_guard<mutex>locker(mtx_);
    level_ = level;
}

void Log::AddLogLevel(int level){
    switch (level)
    {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::AsyncWrite(){
    string str = "";
    while(queue_->dequeue(str)){
        lock_guard<mutex>locker(mtx_);
        fputs(str.c_str(), fd);
    }
}