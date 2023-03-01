/*
 * @Author: zzzzztw
 * @Date: 2023-03-01 14:00:27
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-01 21:31:56
 * @FilePath: /Webserver/http/httpconn.h
 */
#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <cassert>
#include <atomic>

#include "./httprequest.h"
#include "./httpresponse.h"
#include "../buffer/buffer.h"
#include "../pool/sqlconnRAII.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);
    ssize_t read(int *saveError);
    ssize_t write(int *saveError);
    void Close();
    int GetFd()const;
    int GetPort()const;
    const char* GetIP()const;
    sockaddr_in GetAddr()const;
    bool process();
    int ToWriteBytes(){
        return iov_[0].iov_len + iov_[1].iov_len;
    }
    bool IsKeepAlive(){
        return request_.IsKeepAlive();
    }

    static std::atomic<int> userCount_;
    static bool isET_;
    static const char * srcDir_;
private:
    int fd_;// 通信的文件描述符
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;//有几个分散写数组 只有报文的话1， 响应文件内容 2
    struct iovec iov_[2];

    Buffer readBuff_;//读缓冲区，接受并保存请求数据的内容
    Buffer writeBuff_;//写缓冲区，写好的响应头将放入写缓冲区，与根据文件映射得到的响应内容一起由writev分散写给fd_

    HttpRequest request_;
    HttpResponse response_;

};







#endif