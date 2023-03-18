#pragma once
#include <atomic>
#include <stddef.h>
#include <vector>
#include <stdio.h>
#include <iostream>

namespace test
{
    template<typename T, size_t MAX_SIZE = 1024>
    class LockFreeQueueLimit
    {
    private:
        std::atomic<size_t> m_Head;
        std::atomic<size_t> m_Tail;
        std::vector<char> m_Data;
        std::atomic<size_t> m_Size;
        std::atomic<size_t> m_ReadBorder;
        std::atomic<size_t> m_WriteBorder;
    public:
        LockFreeQueueLimit():
        m_Head(0), m_Tail(0), m_Size(0), 
        m_Data((MAX_SIZE + 1) * sizeof(T)),
        m_ReadBorder(0), m_WriteBorder(0)
        {
            static_assert(MAX_SIZE >= 1, "Length of LockFreeQueue must bigger than 1.");
        }

        bool isEmpty() const 
        {
            return m_Head.load() == m_Tail.load();
        }

        size_t Size() const
        {
            return m_Size.load();
        }

        template<typename R>
        bool EnQueue(R&& task)
        {
            T tmp = std::forward<R>(task);
            size_t cur_head;
            size_t cur_tail;

            do{
                cur_tail = m_Tail.load();
                cur_head = m_Head.load();
            }while((cur_tail + 1) % (MAX_SIZE + 1) != cur_head && !m_Tail.compare_exchange_weak(cur_tail, (cur_tail + 1) % (MAX_SIZE + 1)));

            if((cur_tail + 1) % (MAX_SIZE + 1) == cur_head){
                return false;
            }

            // Reader Boarder protect
            while(m_ReadBorder.load() != m_WriteBorder.load() && m_ReadBorder.load() == cur_tail){
                std::cout << "-----------------------" << std::endl;
                std::cout << m_Head.load() << std::endl;
                std::cout << m_WriteBorder.load() << std::endl;
                std::cout << m_Tail.load() << std::endl;
                std::cout << m_WriteBorder.load() << std::endl;
            }

            ::new (reinterpret_cast<T*>(m_Data.data() + sizeof(T) * cur_tail)) T(std::move(tmp));

            while(!m_WriteBorder.compare_exchange_weak(cur_tail, (cur_tail + 1) % (MAX_SIZE + 1)));

            // end Reader Boarder protect

            size_t s;
            do{
                s = m_Size.load();
            }while(!m_Size.compare_exchange_weak(s, s + 1));

            return true;
        }

        bool DeQueue(T& task)
        {
            size_t cur_head;
            size_t cur_tail;

            do{
                cur_head = m_Head.load();
                cur_tail = m_Tail.load();
            }while(cur_head != cur_tail && !m_Head.compare_exchange_weak(cur_head, (cur_head + 1) % (MAX_SIZE + 1)));

            if(cur_head == cur_tail){
                return false;
            }

            // Writer Boarder protect

            while(cur_head >= m_WriteBorder.load());

            task = std::move(
                *reinterpret_cast<T*>(m_Data.data() + sizeof(T) * cur_head)
            );

            while(!m_ReadBorder.compare_exchange_weak(cur_head, (cur_head + 1) % (MAX_SIZE + 1)));

            size_t s;
            do{
                s = m_Size.load();
            }while(!m_Size.compare_exchange_weak(s, s - 1));

            return true;
        }
    };

// #define compare_and_swap __sync_bool_compare_and_swap

//     template<typename T>
//     class LockFreeQueueUnlimit
//     {
//     private:
//         struct Entity
//         {
//             Entity(): next(nullptr){}

//             Entity(const T& val): next(nullptr)
//             {
//                 ::new (reinterpret_cast<T*>(&value[0])) T(val);
//             }
//             Entity(T&& val): next(nullptr)
//             {
//                 ::new (reinterpret_cast<T*>(&value[0])) T(std::move(val));
//             }
//             char value[sizeof(T)];
//             Entity* next;
//         };
//         Entity* m_Tail;
//         Entity* m_Head;
//         std::atomic<size_t> m_Size();
//     public:
//         LockFreeQueueUnlimit()
//         : m_Tail(new Entity()), m_Head(m_Tail), m_Size(0){}

//         bool isEmpty() const 
//         {
//             return m_Head.load() == nullptr;
//         }

//         size_t Size() const
//         {
//             return m_Size.load();
//         }

//         template<typename R>
//         bool EnQueue(R&& task)
//         {
//             Entity* newNode = new Entity(std::forward<R>(task));
//             Entity* p;
//             do{
//                 p = m_Tail;
//             }while(!compare_and_swap(&m_Tail -> next, NULL, newNode));

//             Entity** cur_tail = &m_Tail.load();

//         }

//         bool DeQueue(T& task)
//         {
            
//         }
//     };
}