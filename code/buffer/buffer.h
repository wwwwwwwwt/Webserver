/*
 * @Author: zzzzztw
 * @Date: 2023-02-21 14:29:13
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 13:48:46
 * @FilePath: /Webserver/code/buffer/buffer.h
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <unistd.h> //write read
#include <cassert>
#include <sys/uio.h> //readv
#include <vector>
#include <cstring>
#include <string>
#include <atomic>
#include <algorithm>

class Buffer{
public:
    Buffer(size_t initBufferSize = 1024);
    ~Buffer() = default;


    size_t ReadableBytes()const; //可读字节数
    size_t WriteableBytes()const; // 可写字节数
    size_t PrependableBytes()const; //头部预留字节数

    const char* Peek()const;//第一个可读位置
    void RetrieveAll(); //从buffer里读取所有字节
    void Retrievelen(size_t len); // 从buffer里读取len个字节
    void RetrieveToEnd(const char* end); // 读到指定位置
    std::string RetrieveAlltoStr();

    void Append(const std::string& str);
    void Append(const void* data, size_t len);
    void Append(const char* str, size_t len);
    void Append(const Buffer& buff);


    void EnsureWriteableBytes(size_t len);
    char* BeginWrite(); //返回第一个可写位置
    const char* BeginWriteConst() const;
    void HasWritten(size_t len);//调整写位置idx

    ssize_t ReadFd(int fd, int* savedErrno);
    ssize_t WriteFd(int fd, int* saveErrno);





private:
    char* Beginidx_(){ return &*buffer_.begin(); }//返回buffer起始地址
    const char* Beginidx_()const { return &*buffer_.begin(); };
    void MakeSpace_(size_t len);// 调整buffer空间

private:
    std::vector<char> buffer_;
    std::atomic<size_t> readidx_;
    std::atomic<size_t> writeidx_;
};

#endif // BUFFER_H