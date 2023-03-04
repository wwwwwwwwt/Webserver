/*
 * @Author: zzzzztw
 * @Date: 2023-03-04 16:42:04
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-03-04 17:13:44
 * @FilePath: /Webserver/timer/test.cpp
 */
#include "./heaptimer.h"
#include <iostream>
#include <thread>
#include <chrono>


void foo() {
    std::cout << "foo is called" << std::endl;
}

void bar() {
    std::cout << "bar is called" << std::endl;
}

void baz() {
    std::cout << "baz is called" << std::endl;
}

int main() {

    HeapTimer timer;

    // 向其中添加一些节点
    timer.add(1, 1000, &foo); // 在1秒后执行foo()
    timer.add(2, 2000, &bar); // 在2秒后执行bar()
    timer.add(3, 3000, &baz); // 在3秒后执行baz()
    // 使用一个循环，不断地调用timer.tick()
    while (true) {
        timer.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 每隔0.5秒检查一次
    }

    return 0;
}