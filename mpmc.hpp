#pragma once
#include <vector>
#include <atomic>
#include <optional>
//inline constexpr size_t num_elements = 1'000'000;
inline constexpr size_t num_elements = 1'000'000;

template <typename T>
struct cacheLineAligned {
    static const size_t T_size = sizeof(T);
    static const size_t line_size = 64;
    static const size_t pad = line_size - T_size;
    T data;
    char padding[pad];
};

template <typename T>
struct MPMCQ {
    std::atomic<int> head; // token for each thread
    std::atomic<int> write_idx; // max write index
    std::atomic<int> tail; //
    cacheLineAligned<T> buffer[num_elements];
    cacheLineAligned<std::atomic<int>> flags[num_elements];
    
    MPMCQ(): head(0), write_idx(-1), tail(0) {
        for(int i = 0; i < num_elements; ++i) {
            flags[i].data.store(0);
        }
    }
    
    MPMCQ(MPMCQ&&) = default;
    
    bool write() {
        int pos = head.fetch_add(1);
        if(pos >= num_elements) {
            head = num_elements;
            return false;
        }
        buffer[pos].data = 100;// only write 100 for now
        flags[pos].data.store(1);
        write_idx++;
        return true;
    }
    
    int read() {
        while(1) {
            if (tail.load() >= num_elements) return false;
            while (tail.load() >= write_idx.load()) {
                //std::cout << " spin: idx is " << tail.load() << "| write_idx is " << write_idx.load() << std::endl;
                thread_local int exp = num_elements - 1; 
                if (tail.compare_exchange_strong(exp, num_elements)) {
                    //std::cout << "Returning true for last element " << std::endl;
                    return num_elements;
                }
                exp = num_elements - 1; 
                if(tail.load() == num_elements) return false;
            }
            thread_local int expected = 1;
            thread_local int idx = tail.load();
            for(; idx <= write_idx.load(); ++idx) {
                //std::cout << "idx is " << idx << "| write_idx is " << write_idx.load() << std::endl;
                if(flags[idx].data.compare_exchange_strong(expected,0)) {
                    int val = tail.fetch_add(1);
                    return val + 1;
                }
                expected = 1;
            }
            if(tail.load() == num_elements) return false;
        }
    }
};
