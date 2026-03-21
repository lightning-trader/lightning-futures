// The MIT License (MIT)
//
// Copyright (c) 2015 Jeremy Letang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// port of
// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue

#ifndef MPSC_QUEUE
#define MPSC_QUEUE

#include <atomic>
#include <iostream>

namespace mpsc {

    // A queue node
    template <typename T>
    struct node {
        // the next node in the queue
        std::atomic<node<T>*> next;
        // the optional value of the node
        T* value;

        // crete a node with an otptionnal value
        node(T* v)
            : value(v) {
            this->next.exchange(std::atomic<node<T>*>(nullptr));
        }

        // move constructor of the node
        node(node&& oth)
            : value(oth.v) {
            this->next.exchange(oth.next);
        }
    };

    // mspc queue
    // each items must have a default constructor
    template <typename T>
    struct mpsc_queue {
        // atomic pointer to the head of the queue
        std::atomic<node<T>*> head;
        // pointer to the tail of the queue
        node<T>* tail;

        // default constructor
        // initialize the queue,
        // check a compile time if the value T is default_constructible
        mpsc_queue() {
            auto stub = new node<T>(nullptr);
            this->head.store(stub, std::memory_order_relaxed);
            this->tail = stub;
        }

        // move constructor
        mpsc_queue(mpsc_queue&& oth) {
            this->tail = oth.tail;
            this->head.store(oth.head.load(std::memory_order_acquire), std::memory_order_release);
            oth.tail = nullptr;
        }

        // destroy the queue
        // remove all remaining values
        ~mpsc_queue() {
            if (tail != nullptr) {
                while (this->pop());
                delete this->tail;
            }
        }

        // 娉ㄦ剰锛歝lone() 鎿嶄綔涓嶆槸绾跨▼瀹夊叏鐨勶紝鍙兘鍦ㄥ崟绾跨▼鐜涓嬩娇鐢?
        // 濡傛灉闇€瑕佺嚎绋嬪畨鍏ㄧ殑鍏嬮殕锛岄渶瑕佷娇鐢ㄥ閮ㄥ悓姝ユ満鍒?
        mpsc_queue clone() {
            mpsc_queue<T> new_q;
            // 鍒犻櫎宸插瓨鍦ㄧ殑 stub 鑺傜偣
            delete new_q.tail;
            // 澶嶅埗褰撳墠闃熷垪鐨勭姸鎬侊紙娉ㄦ剰锛氳繖鍙槸娴呮嫹璐濓紝涓嶄繚璇佺嚎绋嬪畨鍏級
            new_q.tail = new node<T>(nullptr);
            new_q.head.store(this->head.load(std::memory_order_acquire), std::memory_order_release);
            return new_q;
        }

        // insert a new value inside the queue
        void push(T* v) {
            auto n = new node<T>(v);
            auto prev = this->head.exchange(n, std::memory_order_acq_rel);
            prev->next.store(n, std::memory_order_release);
        }

        // pop a node from the queue
        // return the result wrapped inside a pop_result
        T* pop() {
            T* ret = nullptr ;
            auto tail = this->tail;
            auto next = tail->next.load(std::memory_order_acquire);
            if (next != nullptr) {
                delete this->tail;
                this->tail = next;
                ret = next->value;
            }
            return ret;
        }

        bool is_empty() const {
            return this->head.load(std::memory_order_acquire) == tail;
        }

    };

}

#endif
