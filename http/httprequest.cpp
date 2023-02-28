/*
 * @Author: zzzzztw
 * @Date: 2023-02-24 14:46:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-28 20:13:13
 * @FilePath: /Webserver/http/httprequest.cpp
 */

#include "httprequest.h"

using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };


void HttpRequest::Init(){
    state_ = REQUEST_LINE;
    method_ = path_ = version_ = body_ = "";
    header_.clear();
    post_.clear();
}

bool HttpRequest::Parse(Buffer &buff){
    if(buff.ReadableBytes() <= 0) return false;
    while(buff.ReadableBytes()&&state_ != FINISH){
        const char * lineEnd = buff.FindCRLF();
        std::string line(buff.Peek(),lineEnd);
        switch(state_){
            case REQUEST_LINE:
                if(!ParseLine_(line)){
                    return false;
                }
                ParsePath_();
                break;
            case HEADERS:
                ParseHeader_(line);
                if(buff.ReadableBytes() <= 2){
                    state_ = FINISH;
                }
                break;
            case BODY:
                ParseBody_(line);
                break;
            default:
                break;
        }
        if(lineEnd == buff.BeginWrite()){break;}
        buff.RetrieveToEnd(lineEnd + 2);
    }

    return true;
}

bool HttpRequest::ParseLine_(const string& line){
     //Get // HTTP/1.1
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");//正则匹配规则
    smatch  submatch;
    if(regex_match(line,submatch,patten)){
        method_ = submatch[1];
        path_ = submatch[2];
        version_ = submatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}

void HttpRequest::ParseHeader_(const string& line){
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];//键值对，：冒号左边为键，冒号右边为值
    }
    else {
        state_ = BODY;//不匹配的时候是遇到了请求头最后结尾的回车换行时/r/n，更换状态
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
}

int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}


std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::IsKeepAlive()const{
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

void HttpRequest::ParsePath_(){
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }    
}

void HttpRequest::ParseURLencode_(){
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':

            //简单的加密操作将中文转换成16转码
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;

            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}


void HttpRequest::ParsePost_() {//表单数据，和登录有关sql
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseURLencode_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(UserVerify_(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

bool HttpRequest::UserVerify_(const string& name,const string& pwd, bool islogin){
    if(name == ""|| pwd == "")return false;
    /*log*/
    MYSQL* sql;
    SqlconnRAII(&sql,SqlConnPool::Instance());

    assert(sql);
    bool flag = false;
    char order[256] = {0};
    MYSQL_RES *res = nullptr;
    unsigned int idx = 0;
    snprintf(order,256,"select username,password from user where username = '%s',limit 1",name.c_str());
    if(mysql_query(sql,order)){
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    idx = mysql_num_rows(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)){
        string password(row[1]);
        /*登录行为且密码正确*/
        if(islogin){
            if( pwd == password){
                flag = true;
            }
            else {
                flag = false;//密码不对
                /**/
            }
        }
        else{//注册行为但username被占用
            flag = false;
        }
    }
    mysql_free_result(res);
    if(!islogin && flag == true){
        /*log*/
        bzero(order,256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        /*LOG_DEBUG( "%s", order);*/
        if(mysql_query(sql, order)) { 
            /*LOG_DEBUG( "Insert error!");*/
            flag = false; 
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
   /* LOG_DEBUG( "UserVerify success!!");*/
    return flag;
}
