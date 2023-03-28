<!--
 * @Author: zzzzztw
 * @Date: 2023-03-08 13:09:22
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-28 18:37:08
 * @FilePath: /Webserver/bug.md
-->
# 遇到的bug

1. epoll注册事件时，将修改EPOLL_CTL_MOD写成了DEL, 导致产生一个连接 注册一个epoll监听就删除一个epoll事件，导致长时间阻塞卡死  
2. 初始化目录文件夹目录拼错，导致一直在连接框旋转，排查很久，从定时器排查到内存映射mmap映射，最终发现是拼错了。。。这类问题很隐蔽，下次应该先排查简单问题再看逻辑。
3. 重大错误，安全队列中一开始使用了这样一种实现，导致服务器无法运行：
```cpp

template<typename T>
void Safequeue<T>::enqueue(T&& t)
{
    std::unique_lock<std::mutex>locker(mtx_);
    condProducer_.wait(locker,[&]{
        if(isClose_)return true;
        return !full();
    });
    if(isClose_)return;
    que_.emplace(t);
    condCumsumer_.notify_all();
}

template<typename T>
bool Safequeue<T>::dequeue(T& t)
{
    std::unique_lock<std::mutex>locker(mtx_);
    condCumsumer_.wait(locker,[&]{
        if(isClose_)return true;
        return !empty();
    });
    if(isClose_)return false;
    t = std::move(que_.front());
    que_.pop();
    condProducer_.notify_all();

    return true;
}

template<typename T>
bool Safequeue<T>::full()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return que_.size() >= capacity_;
}

template<typename T>
bool Safequeue<T>::empty()
{
    std::lock_guard<std::mutex>locker(mtx_);
    return que_.empty();
}
```

之前学习的时候，只知道wait被唤醒或超时会先获取锁lock，根据条件判断是否成立，如果成立就返回，否则释放锁继续休眠。但以上所写显然当wait被唤醒时获取了锁lock，wait时释放了锁，但在进行消息队列是否为空/满时，由于判断满/空的函数也加了锁，从而导致死锁。（锁滥用的情况，下次注意）

4. 不能支持post经过排查bug,原因如下：  
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

