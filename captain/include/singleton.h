#pragma once
namespace captain {
//这个有点问题 ？单例模式必须把类的默认构造函数声明为私有的
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }

/* private:
    Singleton() {} // 将构造函数声明为私有的，禁止外部直接创建实例
    Singleton(const Singleton&) = delete; // 禁用拷贝构造函数
    Singleton& operator=(const Singleton&) = delete; // 禁用赋值运算符 */
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }

/* private:
    SingletonPtr() {} // 将构造函数声明为私有的，禁止外部直接创建实例
    SingletonPtr(const SingletonPtr&) = delete; // 禁用拷贝构造函数
    SingletonPtr& operator=(const SingletonPtr&) = delete; // 禁用赋值运算符 */
};

}
