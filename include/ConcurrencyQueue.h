#pragma once

#include <mutex>
#include <queue>

namespace test
{
template<typename T>
class ConcurrencyQueue{
private:
    std::mutex m_Mutex;
    std::queue<T> m_Queue;
public:
    ConcurrencyQueue(){}
            
    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Queue.empty();
    }
  
    size_t Size(){
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Queue.size();
    }

    template<typename R>
    void EnQueue(R&& task)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.push(std::forward<R>(task));
    }
  
    bool DeQueue(T& task)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(m_Queue.empty()){
            return false;
        }
        task = std::move(m_Queue.front());
        m_Queue.pop();
        return true;
    }
};
}