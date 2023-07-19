#include "InetAddress.h"

#include <string.h>

// 打包ip和端口
InetAddress::InetAddress(uint16_t port, std::string ip){
    // 缓冲区清零
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    // 小端和大端转换
    addr_.sin_port = htons(port);
    // 因为char* 所以转换成c_str()转换成c的字符串格式
    // inet_addr同时也转成了网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

// 转成本地字节序
std::string InetAddress::toIp() const{
    // addr_
    char buf[64] = {0};
    // 网络字节序转成本地字节序
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    // buf转换的东西
    return buf;
}


std::string InetAddress::toIpPort() const{
    // ip:port
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end=strlen(buf);
    // 网络字节序转换为本地字节序
    uint16_t port =ntohs(addr_.sin_port);
    // 最终结果是ip:port
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main(){
//     InetAddress addr(8080);
//     std::cout<< addr.toIpPort()<<std::endl;

//     return 0;
// }
