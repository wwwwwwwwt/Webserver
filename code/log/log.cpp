/*
 * @Author: zzzzztw
 * @Date: 2023-03-09 21:48:40
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-15 18:10:45
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
            std::unique_ptr<std::thread> NewThread(new thread(FlushWritethread));//开一个新线程并执行异步写操作
            writeThread_ = move(NewThread);
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
    gettimeofday(&now,nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list ptr;

    /*不是同一个日子 或 同一个日子行数达到了上限，需要重新建立一个日志文件*/
    if(toDay_ != t.tm_mday ||(lineCount_ && lineCount_ % LOG_LINE_NUM == 0))
    {
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};

        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if(toDay_ != t.tm_mday)
        {
            //不是同一个日子就开一个新日子
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;            
        }
        else{
            //同一个日子但一个日子达到行数上限，滚动日志后面-2-3区分，但行数继续 
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / LOG_LINE_NUM), suffix_);
        }

        unique_lock<mutex>locker(mtx_);
        flush();
        fclose(fd);
        fd = fopen(newFile,"a");
        assert(fd != nullptr);
    }

    /*写日志*/
    {
        unique_lock<mutex>locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AddLogLevel(level);

        va_start(ptr,format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WriteableBytes(), format, ptr);
        va_end(ptr);

        buff_.HasWritten(m);
        buff_.Append("\r\0", 2);
        if(isAsync_ && queue_ && !queue_->full())
        {
            queue_->enqueue(buff_.RetrieveAlltoStr());
        }
        else{
            fputs(buff_.Peek(),fd);
        }
        buff_.RetrieveAll();
    }
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

void Log::FlushWritethread(){
    Log::Instance()->AsyncWrite();
}