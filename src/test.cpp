#include"ThreadPool.h"
#include<iostream>
#include <chrono>
#include "LockFreeQueue.h"

#define __MAX_THREAD_NUM__ 10

size_t usedMem = 0;



// void* operator new(size_t size)
// {
//     std::cout << "acquired: " << size << std::endl;
//     usedMem += size;
//     std::cout << "total used: " << usedMem << std::endl;
//     return malloc(size);
// }

// void operator delete(void* mem, size_t size)
// {
//     std::cout << "freed: " << size << std::endl;
//     usedMem -= size;
//     std::cout << "total used: " << usedMem << std::endl;
//     free(mem);
// }

// void* operator new[](size_t size)
// {
//     std::cout << "acquired: " << size << std::endl;
//     usedMem += size;
//     std::cout << "total used: " << usedMem << std::endl;
//     return malloc(size);
// }

// void operator delete[](void* mem, size_t size)
// {
//     std::cout << "freed: " << size << std::endl;
//     usedMem -= size;
//     std::cout << "total used: " << usedMem << std::endl;
//     free(mem);
// }
class Timer{
private:
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
public:
    Timer()
    :m_Start(std::chrono::high_resolution_clock::now()){}

    Timer(const Timer&) = delete;

    Timer& operator=(const Timer&) = delete;

    ~Timer()
    {
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - m_Start).count() << std::endl;
    }
};

void test0()
{
    test::LockFreeQueueLimit<int, 1> queue;
    queue.EnQueue(1);
    queue.EnQueue(2);
    queue.EnQueue(3);
    int t;
    queue.DeQueue(t);
    std::cout << t << std::endl;
    queue.DeQueue(t);
    std::cout << t << std::endl;
}

void test1()
{
    test::LockFreeQueueLimit<int, 100> queue;
    long long count = 0;
    std::mutex mx;
    auto t1 =  std::thread(
        [&queue, &count, &mx](){
            for(int i = 0;i < 1024; ++i){
                if(i % 2 == 0){
                    while(!queue.EnQueue(i));
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        count += i;
                    }
                }
            }
        }
    );
    auto t2 =  std::thread(
        [&queue, &count, &mx](){
            for(int i = 0;i < 1024; ++i){
                if(i % 2 != 0){
                    while(!queue.EnQueue(i));
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        count += i;
                    }
                }
            }
        }
    );

    auto t3 =  std::thread(
        [&queue, &count, &mx](){
            for(int i = 0;i < 1024; ++i){
                while(!queue.EnQueue(i));
                {
                    std::lock_guard<std::mutex> lc(mx);
                    count += i;
                }
            }
        }
    );

    auto t4 =  std::thread(
        [&](){
            while(!queue.isEmpty()){
                int t;
                if(queue.DeQueue(t)){
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        std::cout << t << std::endl;
                        count -= t;
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lc(mx);
                std::cout << "t4: quite" << std::endl;
            }
        }
    );

    auto t5 =  std::thread(
        [&](){
            while(!queue.isEmpty()){
                int t;
                if(queue.DeQueue(t)){
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        std::cout << t << std::endl;
                        count -= t;
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lc(mx);
                std::cout << "t5: quite" << std::endl;
            }
        }
    );

    auto t6 =  std::thread(
        [&](){
            while(!queue.isEmpty()){
                int t;
                if(queue.DeQueue(t)){
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        std::cout << t << std::endl;
                        count -= t;
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lc(mx);
                std::cout << "t6: quite" << std::endl;
            }
        }
    );

    bool flag = true;

    auto t7 =  std::thread(
        [&](){
            while(flag){
                int t;
                if(queue.DeQueue(t)){
                    {
                        std::lock_guard<std::mutex> lc(mx);
                        std::cout << t << std::endl;
                        count -= t;
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lc(mx);
                std::cout << "t7: quite" << std::endl;
            }
        }
    );
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    std::cout << queue.Size() << std::endl;
    std::cout << count << std::endl;
}

void test2()
{
    using namespace std::chrono_literals;
    test::ThreadPool& pool = test::ThreadPoolFac::Instance();
    // test::ThreadPool pool(std::thread::hardware_concurrency());
    std::vector<std::future<int>> resList1; 
    std::vector<std::future<double>> resList2;
    {
        Timer timer;
        for(int i = 0; i < 200; ++i){
            if(i % 2 == 0){
                auto f = pool.Submit([](int a) -> int {
                    std::this_thread::sleep_for(5s);
                    return a;
                }, i);
                resList1.push_back(std::move(f));
            }else{
                auto f = pool.Submit([](double a) -> double {
                std::this_thread::sleep_for(5s);
                return a;
                }, i);
                resList2.push_back(std::move(f));
            }
            
        }

        for(int i = 0; i < 100; ++i){
            std::cout << resList1[i].get() << std::endl;
            std::cout << resList2[i].get() << std::endl;
        }
    }
    std::cout << "End" << std::endl;
}

int main()
{
    test2();
}
