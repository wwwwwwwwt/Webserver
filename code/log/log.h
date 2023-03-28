/*
 * @Author: zzzzztw
 * @Date: 2023-03-09 18:00:16
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-12 15:31:32
 * @FilePath: /Webserver/code/log/log.h
 */
#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <stdarg.h> //##__VA_ARGS__
#include <sys/stat.h> //mkdir
#include <sys/time.h>
#include <chrono>

#include "./safequeue.h"
#include "../pool/threadpool.h"
#include "../buffer/buffer.h"


class Log{

public:

    void init(int level, const char* path = "./log",  const char* suffix = ".log", int safeQueueCap = 1024);
   
    static Log * Instance(); //单例

    static void FlushWritethread();
    void flush();

    int GetLevel();

    void SetLevel(int level);
    
    void write(int level, const char* format, ...);
    
   
    bool isOpen(){ return isOpen_; };

private:
    Log();
    
    ~Log();

    void AsyncWrite();

    void AddLogLevel(int level);

private:

    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int LOG_LINE_NUM = 50000;

    const char* path_;
    const char* suffix_;

    int lineCount_;
    int toDay_;

    bool isOpen_;

    Buffer buff_;

    int level_;

    bool isAsync_;

    FILE* fd;// fopen, fclose
    std::unique_ptr<std::thread>writeThread_;
    std::unique_ptr<Safequeue<std::string>>queue_;
    std::mutex mtx_;

};

#define LOG_BASE(level, format, ...)\
    do{\
        Log* log = Log::Instance();\
        if(log->isOpen() && log->GetLevel()<= level){\
            log->write(level,format, ##__VA_ARGS__);\
            log->flush();\
        }\
    }while(0);


#define LOG_DEBUG(format,...)do{LOG_BASE(0,format,##__VA_ARGS__)}while(0);
#define LOG_INFO(format,...)do{LOG_BASE(1,format,##__VA_ARGS__)}while(0);
#define LOG_WARN(format,...)do{LOG_BASE(2,format,##__VA_ARGS__)}while(0);
#define LOG_ERROR(format,...)do{LOG_BASE(3,format,##__VA_ARGS__)}while(0);

#endif