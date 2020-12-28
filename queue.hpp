#pragma once
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include <optional>
#include <queue>
#include "mpsc.hpp"
std::mutex mtx;

template <typename T>
struct NaiveQ {
    std::queue<T> queue;
    cacheLineAligned<std::mutex> mtx;
  
    bool try_write(T val) {
        mtx.data.lock();
        queue.push(val);
        mtx.data.unlock();
        return true;
    }
    
    /* only one reader at a time */
    std::optional<T> try_read() {
        mtx.data.lock();
        if(queue.size()) {
            auto val = queue.front();
            queue.pop();
            mtx.data.unlock();
            return val;
        }
        else {
            mtx.data.unlock();
            return {};
        }
    }
};
