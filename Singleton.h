#pragma once
//模板类，本身不作为对象而实例，算是抽象类的一种
#include <iostream>
#include <memory>
#include <mutex>

template<typename T>
class Singleton
{
public:
    static std::shared_ptr<T> GetInstance()
    {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() { m_Instance = std::shared_ptr<T>(new T); });

        return m_Instance;
    }

    ~Singleton() { std::cout << "Singleton Destructed!" << std::endl; }

protected: //能被子类继承
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>& st) = delete;

    static std::shared_ptr<T> m_Instance;
};
