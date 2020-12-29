#pragma once
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include <optional>
#include <queue>
#include "mpsc.hpp"

template <typename T>
struct Node {
    T data;
    Node<T>* next = nullptr;
};

template <typename T>
struct LLQ {
    cacheLineAligned<std::mutex> mtx;
    cacheLineAligned<Node<T>*> head;
    cacheLineAligned<Node<T>*> tail;
    cacheLineAligned<int> size;
    
    LLQ() {
        size.data = 0;
    }
  
    void insert(Node<T> * ptr) {
        std::lock_guard<std::mutex> lk(mtx.data);
        if(size.data == 0) {
            tail.data = ptr;
            head.data = ptr;
            size.data++;
            return;
        }
        tail.data->next = ptr;
        tail.data = ptr;
        size.data++;
        return;
    }
    
    void print() {
        auto start = head.data;
        auto end = tail.data;
        while(start != end) {
            std::cout << start->data.data << "=>";
            start = start->next;
        }
        std::cout << std::endl;
    }
    
    std::optional<T> pop() {
        mtx.data.lock();
        if(size.data == 0) {
            mtx.data.unlock();
            return std::nullopt;
        }
        auto val = head.data->data;
        auto old_head = head.data;
        head.data = head.data->next;
        delete old_head; // FIX THIS LEAK 
        --size.data;
        mtx.data.unlock();
        return val;
    }

    

    bool try_write(T val) {
        Node<T> * n = new Node<T>;
        n->data = val;
        insert(n);
        return true;
    }
    
    /* only one reader at a time */
    std::optional<T> try_read() {
        return pop();
    }
};
