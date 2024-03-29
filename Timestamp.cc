#include "Timestamp.h"
#include <time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){}
// 构造函数初始化列表的语法
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){}
Timestamp Timestamp::now(){
    time_t ti=time(NULL);
    // 获取当前时间
    return Timestamp(ti);
}
std::string Timestamp::toString() const{
    char buf[128]={0};
    tm *tm_time=localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
        // tm结构体里的原因，所以要年+1900，月+1
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
    // C形式的字符串
    return buf;
}

// #include <iostream>
// int main()
// {
//     std::cout << Timestamp::now().toString() << std::endl; 
//     return 0;
// }