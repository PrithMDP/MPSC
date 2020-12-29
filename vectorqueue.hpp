#pragma once
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include <optional>
#include <queue>
#include <tuple>
#include "mpsc.hpp"

template <typename T>
struct VecQ {
    std::vector<T> queue;
    cacheLineAligned<std::mutex> mtx;
    cacheLineAligned<std::tuple<uint64_t,uint64_t,uint64_t>> nums; // write_idx, read_idx, size

    VecQ() {
        queue.reserve(num_elements);
    }
  
    bool try_write(T val) {
        std::lock_guard<std::mutex> lk(mtx.data);
        if(std::get<2>(nums.data) == num_elements) return false;
        queue[std::get<0>(nums.data)] = val;
        std::get<2>(nums.data)++;
        std::get<0>(nums.data)++;
        std::get<0>(nums.data) %= num_elements;
        return true;
    }
    
    /* only one reader at a time */
    std::optional<T> try_read() {
        mtx.data.lock();
        if(std::get<2>(nums.data) == 0) {
            mtx.data.unlock();
            return {};
        }
        else {
            auto val = queue[std::get<1>(nums.data)];
            std::get<1>(nums.data)++;
            std::get<1>(nums.data) %= num_elements;
            std::get<2>(nums.data)--;
            mtx.data.unlock();
            return val;
        }
    }
};
