#pragma once

namespace test
{
namespace utility
{
    template<typename T>
    class Singleton{
    private:
        static T* m_Ptr;
    public:
        Singleton() = delete;
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
    public:
        static T& Instance()
        {
            if(m_Ptr == nullptr){
                m_Ptr = new T();
            }
            return *m_Ptr;
        }
    };

    template<typename T>
    T* Singleton<T>::m_Ptr = nullptr;
}   
}

