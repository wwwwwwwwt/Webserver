/*
 * @Author: zzzzztw
 * @Date: 2023-02-24 14:46:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-24 14:56:21
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

