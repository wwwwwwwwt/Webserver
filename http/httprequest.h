/*
 * @Author: zzzzztw
 * @Date: 2023-02-23 13:18:10
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-23 15:11:39
 * @FilePath: /Webserver/http/httprequest.h
 */
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstring>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"

class HttpRequest{
public:
    HttpRequest();
    ~HttpRequest();


};

#endif 
