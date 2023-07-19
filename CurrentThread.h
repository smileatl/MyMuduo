#pragma once

#include <unistd.h>
#include <sys/syscall.h>

// 获取当前线程tid
namespace CurrentThread
{
    // 声明了一个线程局部存储的整型变量 t_cachedTid
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        // 使用 __builtin_expect 内置函数来提高条件判断的性能
        // __builtin_expect用于给编译器提示条件表达式的预期结果，以便优化生成的代码。
        // 结果很可能是假（0），让编译器去优化
        // 只有一开始可能是0
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
