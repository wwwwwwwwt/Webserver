/*
 * @Author: zzzzztw
 * @Date: 2023-03-04 13:11:11
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-07 22:15:51
 * @FilePath: /Webserver/server/webserver.h
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "./epoller.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
#include "../pool/threadpool.h"
#include "../timer/heaptimer.h"

class Webserver{
public:
    Webserver(
        int port, int trigMod, int timeoutMs, bool optlinger, int sqlport, const char* sqlUser, const char * sqlpwd,
        const char* dbname, int connPoolnum, int threadnum);
    ~Webserver();
    void start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMod);
    void AddClient_(int fd, sockaddr_in addr);
    int  SetNoBlock(int fd);
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_( int fd, const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn * client);
    void OnProcess_(HttpConn * client);

    static const int MAX_FD  = 65536;

    int port_; //端口号
    int timeoutMs_;//连接超时时间
    bool openLinger_; // 优雅关闭
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<ThreadPool>threadpool_;
    std::unique_ptr<Epoller>epoller_;
    std::unique_ptr<HeapTimer>timer_;
    std::unordered_map<int,HttpConn>users_;

    
};

#endif