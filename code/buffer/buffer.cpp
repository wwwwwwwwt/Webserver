/*
 * @Author: zzzzztw
 * @Date: 2023-02-21 16:15:58
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 13:48:17
 * @FilePath: /Webserver/code/buffer/buffer.cpp
 */

#include "buffer.h"


Buffer::Buffer(size_t initBufferSize):buffer_(initBufferSize),readidx_(0),writeidx_(0){}

size_t Buffer::ReadableBytes()const{

    return writeidx_ - readidx_;
}

size_t Buffer::WriteableBytes()const{
    return buffer_.size() - writeidx_;
}

size_t Buffer::PrependableBytes()const{
    return readidx_;
}

const char* Buffer::Peek()const{
    return Beginidx_() + readidx_;
}

void Buffer::RetrieveAll(){
    bzero(&buffer_[0],buffer_.size());
    readidx_ = 0;
    writeidx_ = 0;
}

void Buffer::Retrievelen(size_t len){
    assert( len <= ReadableBytes());
    readidx_ += len;
}

void Buffer::RetrieveToEnd(const char* end){
    assert(Peek() <= end );
    Retrievelen(end - Peek());
}

std::string Buffer:: RetrieveAlltoStr(){
    std::string str(Peek(),ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer:: Append(const std:: string &str){
    Append(str.data(),str.size());
}

void Buffer:: Append(const void* data,size_t len){
    assert(data);
    Append(static_cast<const char*>(data),len);
}

void Buffer:: Append(const char* data, size_t len){
    assert(data);
    EnsureWriteableBytes(len);
    std::copy(data,data + len , BeginWrite());
    HasWritten(len);
}

void Buffer:: Append(const Buffer& buf){
    Append(buf.Peek(),buf.ReadableBytes());
}

void Buffer::EnsureWriteableBytes(size_t len){
    if(WriteableBytes() < len){
        MakeSpace_(len);
    }
    assert(WriteableBytes() >= len);
}

char* Buffer::BeginWrite(){
    return Beginidx_() + writeidx_;
}

const char* Buffer::BeginWriteConst() const{
    return Beginidx_() + writeidx_;
}

void Buffer::HasWritten(size_t len){
    assert(len >= 0);
    writeidx_ += len;
}

void Buffer::MakeSpace_(size_t len){
    assert(len >= 0);
    if(WriteableBytes() + PrependableBytes() < len ) {
        buffer_.resize(writeidx_ + len + 1);
    }
    else{
        const size_t readable = ReadableBytes();
        std::copy(Beginidx_() + readidx_, Beginidx_() + writeidx_, Beginidx_());
        readidx_ = 0;
        writeidx_ = readidx_ + readable;
        assert(ReadableBytes() == readable);
    }
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno){
    char tempbuf[65536];
    struct iovec vec[2];
    const size_t writable = WriteableBytes();

    vec[0].iov_base = Beginidx_() + writeidx_;
    vec[0].iov_len = writable;
    vec[1].iov_base = tempbuf;
    vec[1].iov_len = sizeof(tempbuf);
  //  const ssize_t iovcnt = (writable < sizeof tempbuf) ? 2:1;//65536 字节已经可以满足千兆网络  0.5微秒内全速收到的数据 65KB/0.5μs = 125MB/s
    const ssize_t len = readv(fd,vec,2);

    if(len < 0){//出错
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable){//读的长度小于可读长度，临时buf中无数据
        writeidx_ += len; 
    }
    else{
        writeidx_ = buffer_.size();//buffer_满了，需要扩容从临时buf中继续读
        Append(tempbuf,len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno){
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0){
        *saveErrno = errno;
        return len;
    }
    readidx_ += len;
    return len;
}
