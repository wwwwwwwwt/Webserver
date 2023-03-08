/*
 * @Author: zzzzztw
 * @Date: 2023-03-05 16:15:26
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 08:51:47
 * @FilePath: /Webserver/code/server/webserver.cpp
 */

#include "webserver.h"

Webserver::Webserver(
        int port, int trigMod, int timeoutMs, bool optlinger, 
        int sqlport, const char* sqlUser, const char * sqlpwd,
        const char* dbname, int connPoolnum, int threadnum)
        :port_(port),timeoutMs_(timeoutMs),openLinger_(optlinger),threadpool_(new ThreadPool(threadnum)),epoller_(new Epoller()),timer_(new HeapTimer())
    {   
        srcDir_ = getcwd(nullptr,256);
        assert(srcDir_ != nullptr);
        strncat(srcDir_,"/resources/",16);

        HttpConn::userCount_ = 0;
        HttpConn::srcDir = srcDir_;

        SqlConnPool::Instance()->Init("localhost",sqlport,sqlUser,sqlpwd,dbname,connPoolnum);
        
        InitEventMode_(trigMod);

        if(!InitSocket_()){
            isClose_ = true;
        }
        
    }

Webserver::~Webserver(){
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();

}

void Webserver::start(){
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { /*loh*/ }
    while(!isClose_) {//循环监听有无事件到达
        //解决超时连接
        if(timeoutMs_> 0) {
            timeMS = timer_->GetNextTick();
        }

        //检测多少个事件发生
        int eventCnt = epoller_->Wait(timeMS);//阻塞一个最小堆根节点的剩余时间
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */

            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                DealListen_();//处理监听的操作，接收客户端连接进来
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {//事件产生错误信号关闭连接
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);//加入到线程池threadpool->onread处理读操作
            }
            else if(events & EPOLLOUT) {//循环检测读事件状态有没有写的权限
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);//加入到线程池threadpool->onwrite处理写操作
            } else {
                /*log*/
            }
        }
    }    
}


bool Webserver::InitSocket_(){

    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 0){
        /*log*/
        return false;
    }
    /*初始化套接字地址格式*/
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct linger optlinger = {0,0};

    if(openLinger_){
        optlinger.l_onoff = 1;//开启 非0， 关闭 0
        /*socket如果阻塞，close()将等待l_linger这么久的时间让tcp缓冲区继续发送消息，之后根据返回值和errno判断有无发送完全
         *如果非阻塞，close()将立即返回，根据返回值和errno判断数据发送完全
         */
        optlinger.l_linger = 1;
    }

    /*三个参数 ipv4, 字节流指定tcp, 前两个参数传入后第三个一般默认0*/
    listenFd_ = socket(AF_INET,SOCK_STREAM,0);

    if(listenFd_ < 0){
        /*log*/

        return false;
    }

    /* 设置优雅关闭 */
    ret = setsockopt(listenFd_,SOL_SOCKET, SO_LINGER, &optlinger, sizeof(optlinger));
    if(ret < 0){
        /*log*/
        close(listenFd_);
        return false;
    }

    /*设置端口复用，如果端口处于Timewait将强制使用Time_wait占用的端口。防止服务器意外关闭，再启动时端口号处于time_wait状态，端口号被占用不能启动
     * 此时将可能收到上一次迟到的报文，但有三次握手保证建立新的连接。
    */
    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if(ret < 0){
        /*log*/
        close(listenFd_);
        return false;
    }

    /*设置好主动套接字socket后，进行绑定*/
    ret = bind(listenFd_, (const sockaddr*)&addr, sizeof(addr));
    if(ret < 0){
        /*log*/
        close(listenFd_);
        return false;
    }

    /*设置监听，将主动套接字转变为监听套接字*/
    /*第二个参数只表示全连接队列上线，全连接队列中的套接字会被accept()取出*/
    /*半连接队列由内核参数 /proc/sys/net/ipv4/tcp_max_backlog定义 */
    ret = listen(listenFd_, 10);
    if(ret < 0){
        /*log*/
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenEvent_|EPOLLIN);
    if(ret == 0){
        /*log*/
        close(listenFd_);
        return false;
    }

    /*设置监听套接字非阻塞*/

    ret = SetNoBlock(listenFd_);
    if(ret  <  0){
        /*log*/
        close(listenFd_);
        return false;
    }

    return true;
}

int Webserver::SetNoBlock(int fd){

    assert(fd > 0);
    int flag = fcntl(fd,F_GETFD,0);
    flag |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flag) ;

}

// 设置监听描述符和通信描述符的通信模式状态
void Webserver::InitEventMode_(int trigMod)
{
    listenEvent_ = EPOLLRDHUP;
    //注册epollooneshot 使得一个socket一次只能被一个线程所处理，即使这个socket上又来了事件也不处理，处理后需要再重新注册epolloneshot，在onewrite用epoll_ctl
    connEvent_ = EPOLLONESHOT|EPOLLRDHUP; 

    switch(trigMod){
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET_ = (connEvent_ & EPOLLET); // 是边缘触发就需要一次性读入/写入缓冲区的全部内容
}

void Webserver::CloseConn_(HttpConn* client){
    assert(client);
    /*log*/

    epoller_->DelFd(client->GetFd());
    client->Close();
}

void Webserver::AddClient_(int fd, sockaddr_in addr){
    assert(fd > 0);

    users_[fd].init(fd, addr);
    if(timeoutMs_ > 0){
        timer_->add(fd,timeoutMs_,std::bind(&Webserver::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, connEvent_ | EPOLLIN);
    SetNoBlock(fd);

    /*log*/
}

void Webserver :: DealListen_(){
    /*获取连接套接字*/
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0){
            return;
        }
        else if(HttpConn::userCount_ >= MAX_FD){
            SendError_(fd, "server busy");
            /*LOG*/
            return;
        }
        AddClient_(fd,addr);
    }while(listenEvent_ & EPOLLET);
    /*dowhile因为et模式监听文件描述符可能收到了两个客户端连接，不循环一次读出下次不会通知，所以需要一次性读出所有客户端的文件描述符直到没有链接fd=-1就退出循环*/
}

void Webserver::DealRead_(HttpConn* client){
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&Webserver::OnRead_,this,client));
}

void Webserver:: DealWrite_(HttpConn* client){
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&Webserver::OnWrite_, this, client));
}

void Webserver::ExtentTime_(HttpConn* client){
    assert(client);
    if(timeoutMs_ > 0) {timer_->adjust(client->GetFd(), timeoutMs_);}
}

void Webserver::OnRead_(HttpConn* client){
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN){//EGAIN常发生在非阻塞下，当read没有数据可读，程序不会阻塞会返回EAGAIN，表示发生了错误，关闭连接
        CloseConn_(client);
        return;
    }
    OnProcess_(client);
}

void Webserver::OnWrite_(HttpConn* client){
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0){
        if(client->IsKeepAlive()){
            OnProcess_(client);
            return;
        }
    }
    else if( ret < 0){
        if(writeErrno == EAGAIN){
            epoller_->ModFD(client->GetFd(), connEvent_| EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

void Webserver::OnProcess_(HttpConn* client){
    if(client->process()){
        epoller_->ModFD(client->GetFd(), connEvent_ | EPOLLOUT|EPOLLONESHOT);
    }
    else{
        epoller_->ModFD(client->GetFd(), connEvent_ | EPOLLIN |EPOLLONESHOT);
    }
}

void Webserver::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
       /*log*/
    }
    close(fd);
}