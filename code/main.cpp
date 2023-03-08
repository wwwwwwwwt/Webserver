/*
 * @Author: zzzzztw
 * @Date: 2023-02-17 13:01:41
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-07 22:43:31
 * @FilePath: /Webserver/code/main.cpp
 */

#include "./server/webserver.h"

int main(){
    /*守护进程*/
    //demon(1,0);

    Webserver server(
        1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "root", "123456", "yourdb", /* Mysql配置 */
        12, 6);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.start();
    
}