/*
 * @Author: zzzzztw
 * @Date: 2023-03-01 14:58:02
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 09:51:58
 * @FilePath: /Webserver/code/http/httpconn.cpp
 */
#include "./httpconn.h"
using namespace std;


const char* HttpConn::srcDir;
atomic<int> HttpConn::userCount_;
bool HttpConn::isET_;

HttpConn::HttpConn(){
    fd_ = -1;
    addr_ = {0};
    iovCnt_ = -1;
    isClose_ = true;
}

void HttpConn::init(int fd, const sockaddr_in& addr){
    assert(fd > 0);

    addr_ = addr;
    fd_ = fd;
    userCount_++;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClose_ = false;
    /*log*/

                        
}

void HttpConn::Close(){
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true;
        userCount_--;
        close(fd_);
    /*log*/std::cout<<"closefd"<<std::endl;
    }
}

HttpConn::~HttpConn(){
    Close();
}

int HttpConn::GetFd()const {
    return fd_;
}

int HttpConn::GetPort()const {
    return addr_.sin_port;
}

sockaddr_in HttpConn::GetAddr()const{
    return addr_;
}

const char* HttpConn::GetIP()const {
    return inet_ntoa(addr_.sin_addr);
}

ssize_t HttpConn::read(int *saveError){ // 将通信用的文件描述符发来的请求报文分散读进 读缓冲区
    int len = -1;
    do{
        len = readBuff_.ReadFd(fd_,saveError);
        if(len <= 0)break;
    }while(isET_);
    return len;
}
bool HttpConn::process(){//进行对请求和响应的处理
    request_.Init();//清空请求报文的成员数据
    if(readBuff_.ReadableBytes() <= 0){ // 当前读缓存没有可读的，报错
        return false;
    }
    else if(request_.Parse(readBuff_)){//根据请求状态机依次解析请求报文，将资源/路径/是否保持连接/成功码 传入响应报文初始化
        /*log*/
        cout<<request_.path().c_str()<<" "<<endl;
        response_.Init(srcDir,request_.path(),request_.IsKeepAlive(),200);
    }
    else{
        response_.Init(srcDir,request_.path(),false,400);
    }

    //根据状态码生成响应报文
    response_.MakeSponse(writeBuff_);

    /*
        响应报文头
    */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /*
        响应报文具体文件内容
    */
    if(response_.FileLen() > 0 && response_.File()){
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    /*log*/
    return true;

}
ssize_t HttpConn::write(int *saveError){// 响应报文，将写缓冲区的消息一次性发给用于通信的文件描述符
    ssize_t len = -1;
    do{
        len = writev(fd_,iov_,iovCnt_);
        if(len <= 0){
            *saveError = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0){break;}
        else if(static_cast<size_t>(len) > iov_[0].iov_len ){
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + len - iov_[0].iov_len;
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len){
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrievelen(len); 
        }
    }while(isET_||ToWriteBytes() > 10240);
    return len;
}

