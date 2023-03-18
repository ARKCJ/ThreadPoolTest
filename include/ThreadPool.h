#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <stddef.h>
#include <future>
#include <chrono>
#include <random>
#include <ctime>
#include <iostream>
#include "ConcurrencyQueue.h"
#include "Singleton.h"

namespace test
{
    class ThreadPool{
    public:
        template<typename T>
        class LocalQueue : public ConcurrencyQueue<T>{
        public:
            LocalQueue(){}
            using ConcurrencyQueue<T>::isEmpty;
            using ConcurrencyQueue<T>::Size;
            using ConcurrencyQueue<T>::EnQueue;
            using ConcurrencyQueue<T>::DeQueue;

            bool TrySteal(T& task)
            {
                return DeQueue(task);
            }
        };
    private:
        size_t m_ThreadNum;
        std::atomic<bool> m_Shutdown;
        ConcurrencyQueue<std::function<void()>> m_Queue;
        std::vector<std::thread> m_Works;
        std::condition_variable empty;
        std::condition_variable s_cv;
        std::mutex m_Mutex;
        std::vector<std::unique_ptr<
            LocalQueue<
                std::function<void()>
            >
        >> m_LocalQueues;
        std::thread m_Scheduler;
        size_t m_MaxQueueSize;
    public:
        ThreadPool(size_t maxThread, size_t maxQueueSize = 10)
        :m_ThreadNum(maxThread),
        m_Works(m_ThreadNum),
        m_Shutdown(false),
        m_LocalQueues(m_ThreadNum),
        m_MaxQueueSize(maxQueueSize)
        { init(); }
        
        ThreadPool(const ThreadPool &) = delete;

        ThreadPool& operator=(const ThreadPool &) = delete;
    private:
        class Worker{
        private:
            size_t m_ID;
            ThreadPool* m_Pool;
        public:
            Worker(ThreadPool* pool, const size_t id)
            :m_Pool(pool), m_ID(id){}

            void operator()()
            {
                std::function<void()> task;
                bool flag;
                while(!m_Pool -> m_Shutdown){
                    flag = false;
                    if(!m_Pool -> m_LocalQueues[m_ID] -> isEmpty()){
                        flag = m_Pool -> m_LocalQueues[m_ID] -> DeQueue(task);
                        // std::cout << "Thread " << m_ID << " Flag " << flag << std::endl;
                    }else if(!m_Pool -> m_Queue.isEmpty()){
                        flag = m_Pool -> m_Queue.DeQueue(task);
                    }else{
                        for(int i = (m_ID + 1) % m_Pool -> m_ThreadNum; 
                        i != m_ID; 
                        i = (i + 1) % m_Pool -> m_ThreadNum){
                            if(!m_Pool -> m_LocalQueues[i] -> isEmpty()){
                                flag = m_Pool -> m_LocalQueues[i] -> TrySteal(task);
                                if(flag != false){
                                    // std::cout << "Thread " << m_ID << " stealed from thread " << i << std::endl;
                                    break;
                                }
                            }
                        }
                        if(flag == false){
                            std::unique_lock<std::mutex> lock(m_Pool -> m_Mutex);
                            m_Pool -> empty.wait(lock);
                        }
                    }
                    if(flag == true){
                        task();
                    }
                }
            }
        };

        class Scheduler{
        private:
            ThreadPool* m_Pool;
            int SchduleIndex()
            {
                return std::rand() % (m_Pool -> m_ThreadNum);
            }
        public:
            Scheduler(ThreadPool* m_Ptr):m_Pool(m_Ptr){}
            
            void operator()()
            {
                using namespace std::chrono_literals;
                std::function<void()> task;
                while(!m_Pool -> m_Shutdown){
                    if((m_Pool -> m_Queue).Size() < m_Pool -> m_MaxQueueSize){
                        std::unique_lock<std::mutex> lock(m_Pool -> m_Mutex);
                        (m_Pool -> s_cv).wait_for(lock, 20s);
                        // std::cout << "Scheduler has been waken up!" << std::endl;
                    }
                    // std::this_thread::sleep_for(10s);
                    size_t i = 0;
                    while(!m_Pool -> m_Queue.isEmpty() && i < m_Pool -> m_MaxQueueSize){
                        if(m_Pool -> m_Queue.DeQueue(task)){
                            int index = SchduleIndex();
                            m_Pool -> m_LocalQueues[index] -> EnQueue(task);
                            // std::cout << "A task from queue to localqueue -> " << index << std::endl;
                        }
                        ++i;
                    }
                    (m_Pool -> empty).notify_all();
                }
            }
        };

    public:
        void Shutdown() 
        {
            m_Shutdown = true;
            empty.notify_all();
            s_cv.notify_all();
            for (int i = 0; i < m_Works.size(); ++i) {
                if(m_Works[i].joinable()) {
                    m_Works[i].join();
                }
            }
            if(m_Scheduler.joinable()){
                m_Scheduler.join();
            }
        }

        ~ThreadPool()
        {
            if(m_Shutdown == false){
                Shutdown();
            }
        }

        template<typename F, typename... Args>
        auto Submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
        {
            auto s_ptr = std::make_shared<
                std::packaged_task<decltype(f(args...))()>
                >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );
            std::function<void()> wrapper = [s_ptr](){ (*s_ptr)(); };
            m_Queue.EnQueue(std::move(wrapper));
            if(m_Queue.Size() >= m_MaxQueueSize){
                s_cv.notify_one();
                // std::cout << "Weak up scheduler." << std::endl;
            }
            empty.notify_one();
            return s_ptr -> get_future();
        }
    private:
        void init() {
            for(int i = 0; i < m_ThreadNum; ++i) {
                m_LocalQueues[i] = std::make_unique<LocalQueue<std::function<void()>>>();
            }
            for(int i = 0; i < m_ThreadNum; ++i) {
                m_Works[i] = std::thread(Worker(this, i));
            }
            m_Scheduler = std::thread(Scheduler(this));
        }
    };

    class ThreadPoolFac
    {
    private:
        static ThreadPool* m_Ptr;
    public:
        ThreadPoolFac() = delete;
        ThreadPoolFac(const ThreadPoolFac&) = delete;
        ThreadPoolFac& operator=(const ThreadPoolFac&) = delete;
    public:

        static void Init(size_t maxThread, size_t maxQueueSize)
        {
            if(m_Ptr == nullptr){
                m_Ptr = new ThreadPool(maxThread, maxQueueSize);
            }
        }

        static ThreadPool& Instance()
        {
            if(m_Ptr == nullptr){
                Init(std::thread::hardware_concurrency(), std::thread::hardware_concurrency() * 2);
            }
            return *m_Ptr;
        }
    };

    ThreadPool* ThreadPoolFac::m_Ptr = nullptr;

}