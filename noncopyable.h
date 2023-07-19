#pragma once

/*
noncopyable被继承以后，派生类对象可以正常的构造和析构，
但是派生类对象无法进行拷贝构造和赋值操作。

通过继承noncopyable类，可以将子类设计为不可拷贝的，
从而提供更好的类封装和语义约束。
这在设计单例模式等场景中很常见。
*/
class noncopyable {
public:
    // 禁止拷贝的技巧，通常用于将一个类设计为不可拷贝的
    // 以防止意外的对象拷贝，确保类的实例不能被复制。
    // 它有一个拷贝构造函数和赋值运算符，且都被删除了（= delete）
    // 这样一来，任何尝试拷贝或赋值该类的实例都会导致编译错误。
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};