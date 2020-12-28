#pragma once
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include <optional>
//std::mutex mtx;
inline constexpr size_t num_elements = 100'000;

template <typename T>
struct cacheLineAligned {
    static const size_t T_size = sizeof(T);
    static const size_t line_size = 64;
    static const size_t pad = line_size - T_size;
    T data;
    char padding[pad];
};

template <typename T>
struct MPSCQ {
    cacheLineAligned<std::atomic<int>> head; // token for each write
    cacheLineAligned<std::atomic<int>> tail; // token for each read
    
    cacheLineAligned<T> buffer[num_elements];
    cacheLineAligned<std::atomic<int>> flags[num_elements];

    
    MPSCQ(){
        head.data = 0;
        tail.data = 0;
        for(int i = 0; i < num_elements; ++i) {
            flags[i].data.store(0);
        }
    }
    
    MPSCQ(MPSCQ&&) = default;

    // need to protect writers from writers
    // set ready to read and increase write_idx
  
    bool try_write(T val) {
        int pos = head.data.load(std::memory_order_acquire);
        int pos_copy = pos;
        pos = pos % num_elements;
        int expected = 0; 
        if(flags[pos].data.compare_exchange_strong(expected, 1)){
            if(pos_copy != head.data.load(std::memory_order_acquire)) { 
                flags[pos].data = 0;
                return false;
            }
            head.data.store(head.data.load() + 1, std::memory_order_release);
            buffer[pos].data = val;
            flags[pos].data.store(2);
            return true;
        }
        else {
            return false;
        }
    }
    
    /* only one reader at a time */
    std::optional<T> try_read() {
        int read_pos = tail.data.load(std::memory_order_acquire);
        read_pos = read_pos % num_elements;
        if (!(flags[read_pos].data.load() == 2)) { return {}; }
        auto ret = buffer[read_pos];
        flags[read_pos].data.store(0);
        tail.data.store(tail.data.load() + 1, std::memory_order_release);
        // std::cout << "successfully read " << read_pos << std::endl;
        return ret.data;
    }
};
