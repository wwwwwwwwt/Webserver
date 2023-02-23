
#include "./httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse(){
    code_ = -1;
    isKeepalive_ = false;
    path_ = srcDir_ = "";
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

HttpResponse::~HttpResponse(){
    UnmapFile(); 
}

void HttpResponse::Init(const string& srcDir, string& path, bool isKeepalive,int code){
    assert(srcDir != "");
    path_ = path;
    isKeepalive_ = isKeepalive;
    code_ = code;
    if(mmFile_){UnmapFile();}
    mmFile_ = nullptr;
    mmFileStat_ = {0};

}

/**
 * MakeSponse 获取请求资源文件的信息，组装响应报文，并在AddContent中取得文件的映射，保存至mmFile_
 * 通过HttpResponse对象的 File(),FileLen()获得映射区地址和长度
*/
void HttpResponse::MakeSponse(Buffer& buff){
    if(stat((srcDir_ + path_).data(),&mmFileStat_)<0 || S_ISDIR(mmFileStat_.st_mode)){
        code_ = 404;
    }
    else if((mmFileStat_.st_mode & S_IROTH) == 0){
        code_ = 403;
    }
    else if(code_ == -1){
        code_ = 200;
    }

    Errorhtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

void HttpResponse::UnmapFile(){
    if(mmFile_){
        assert(munmap(mmFile_, mmFileStat_.st_size));
        mmFile_ = nullptr;
    }
}

char * HttpResponse::File(){
    return mmFile_;
}

size_t HttpResponse::FileLen()const {
    return static_cast<size_t>(mmFileStat_.st_size);
}


/*
 * 根据哈希表中键值对向缓存区中添加错误的报文内容，message为报错响应内容   
 */
void HttpResponse::ErrorContent(Buffer& buff,string message){
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }
    else{
        status = "Bad Request";
    }

    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

/*
 *向缓存区buff中添加报文头信息
 */
void HttpResponse::AddStateLine_(Buffer &buff){
    string status;
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }
    else{
        code_ = 400;
        status = CODE_STATUS.find(code_)->second;
    }

    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n" );

}

void HttpResponse::AddHeader_(Buffer &buff){
    buff.Append("Connection: ");
    if(isKeepalive_){
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer &buff){
    int srcfd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcfd < 0){
        ErrorContent(buff, "File NotFound!");
        return;
    }

    /**
     * 建立一个内存映射提高文件访问速度，0代表由系统决定映射区的开始位置，第二个参数文件大小，第三个参数只读，第四个参数映射对象类型MAP_PRIVATE为写时拷贝
     * 第五个文件描述符 一般由open得到，第六个为被映射对象内容的起点偏移量。
     */
    int* mmRet = static_cast<int*>(mmap(0,mmFileStat_.st_size,PROT_READ,MAP_PRIVATE,srcfd,0));

    if(*mmRet == -1){
        ErrorContent(buff,"File NotFound!");
    }

    mmFile_ = reinterpret_cast<char *>(mmRet);//映射 在httpconn分散写

    close(srcfd);
    mmRet = nullptr;
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");

}

void HttpResponse::Errorhtml_() {
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

string HttpResponse::GetFileType_() {
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}