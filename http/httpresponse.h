/*
 * @Author: zzzzztw
 * @Date: 2023-02-22 15:00:12
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-22 21:29:38
 * @FilePath: /Webserver/http/httpresponse.h
 */
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <unordered_map>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap munmap
#include <sys/stat.h> //stat
#include <assert.h>
#include "../buffer/buffer.h"

class HttpResponse{

public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepalive = false, int code = -1);
    void MakeSponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t FileLen()const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code()const {return code_;}


private:   
    void AddStateLine_(Buffer &buff);
    void AddHeader_(Buffer &buff);
    void AddContent_(Buffer &buff);
    void Errorhtml_();
    std::string GetFileType_();

private:
    int code_; //状态码
    bool isKeepalive_; //是否长连接

    std::string path_; // 资源的路径
    std::string srcDir_;//资源的目录
    char* mmFile_; //文件内存映射指针
    struct stat mmFileStat_; //文件状态信息

    static const std::unordered_map<std:: string, std::string> SUFFIX_TYPE;//后缀的类型
    static const std::unordered_map<int, std::string>CODE_STATUS; //状态码-描述
    static const std::unordered_map<int, std::string>CODE_PATH; //状态码-页面路径

};




#endif 