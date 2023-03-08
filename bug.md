<!--
 * @Author: zzzzztw
 * @Date: 2023-03-08 13:09:22
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-08 13:58:30
 * @FilePath: /Webserver/bug.md
-->
# 遇到的bug

1. epoll注册事件时，将修改EPOLL_CTL_MOD写成了DEL, 导致产生一个连接 注册一个epoll监听就删除一个epoll事件，导致长时间阻塞卡死  
2. 初始化目录文件夹目录拼错，导致一直在连接框旋转，排查很久，从定时器排查到内存映射mmap映射，最终发现是拼错了。。。这类问题很隐蔽，下次应该先排查简单问题再看逻辑。
1. 不能支持post经过排查bug,原因如下：  
buffer缓冲区中设定了一个寻找"\r\n"的函数，如果到了缓冲区末尾就返回nullptr,在request::parse状态机中使用,返回参数交给lienEnd保存，截取其中的内容进行解析，但忽略了到了末尾，返回了nullptr，导致string拷贝构造函数报错，代码如下
```cpp

const char* Buffer::FindCRLF()const{
    const char CRLF[] = "\r\n";
    const char* crlf = std::search(Peek(),BeginWriteConst(),CRLF,CRLF+2);
    return crlf == BeginWriteConst() ? Peek():crlf;
}

const char* lineEnd = std::search(buff.Peek(),buff.BeginWriteConst(),CRLF,CRLF+2);
//const char * lineEnd = buff.FindCRLF();
std::string line(buff.Peek(),lineEnd);
```


```  what():  basic_string::_M_create```
导致解析请求体时，直接报错，跳过body,这样除了登陆注册时一切安好，登录注册时需要解析请求体会报错
2. sql语句小写了无法登录