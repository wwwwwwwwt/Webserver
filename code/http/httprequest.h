/*
 * @Author: zzzzztw
 * @Date: 2023-02-23 13:18:10
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-28 20:13:05
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
#include "../pool/sqlconnRAII.h"
#include "../pool/sqlconnpool.h"

class HttpRequest{
public:
    enum PARSE_STATE {
        REQUEST_LINE,//正在解析请求首行
        HEADERS,//头
        BODY,//体
        FINISH,  //完成      
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,//没有请求
        GET_REQUEST,//收到请求
        BAD_REQUEST,//错误请求
        NO_RESOURSE,//没有资源
        FORBIDDENT_REQUEST,//禁止访问
        FILE_REQUEST,//请求一个文件
        INTERNAL_ERROR,//内部错误
        CLOSED_CONNECTION,//连接关闭
    };
    HttpRequest(){ Init(); };
    ~HttpRequest() = default;
    void Init();
    bool Parse(Buffer &buff);//根据状态机解析报文

    std::string path()const;
    std::string& path();
    std::string method()const;
    std::string version()const;
    std::string GetPost(const std::string& key)const;
    std::string GetPost(const char* key)const;
    bool IsKeepAlive() const;
private:

    bool ParseLine_(const std::string& line);//解析请求行
    void ParseHeader_(const std::string& line);//解析请求头
    void ParseBody_(const std::string& line);//解析请求体
    void ParsePath_();//解析资源路径
    void ParseURLencode_();//解析用户名密码
    bool UserVerify_(const std::string &name, const std::string &pwd, bool islogin);//验证
    void ParsePost_();//解析用户登录注册信息


    PARSE_STATE state_;//解析的状态
    std::unordered_map<std::string, std::string>header_;
    std::unordered_map<std::string, std::string>post_;
    std::string method_,path_,version_,body_;

    static const std::unordered_set<std::string> DEFAULT_HTML;//默认的网页目录
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;//注册或登录二级网页
    static int ConverHex(char ch);
};

#endif 
