#pragma once
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include <optional>
inline constexpr size_t num_elements = 100'000;

template <typename T>
struct alignas(64) cacheLineAligned {
    static const size_t T_size = sizeof(T);
    static const size_t line_size = 64;
    static const size_t pad = line_size - T_size;
    T data;
    char padding[pad];
};

template <typename T>
struct MPSCQ {
    cacheLineAligned<std::atomic<uint64_t>> head; // token for each write
    cacheLineAligned<std::atomic<uint64_t>> tail; // token for each read
    
    cacheLineAligned<T> buffer[num_elements];
    cacheLineAligned<std::atomic<uint64_t>> flags[num_elements];
    cacheLineAligned<std::atomic<int>> version[num_elements];

    
    MPSCQ(){
        head.data = 0;
        tail.data = 0;
        for(size_t i = 0; i < num_elements; ++i) {
            flags[i].data.store(0);
            version[i].data.store(-1);
        }
    }
    
    MPSCQ(MPSCQ&&) = default;

    /*
        This will block till the current spot has been read from and till the previous writer has finished.
        This is needed to ensure FIFO ordering.
    */
  
    bool try_write(T val) {
        uint64_t pos = head.data.fetch_add(1);
        uint64_t c_version = pos / num_elements;
        pos = pos % num_elements;
        int expected_version = c_version - 1;
        while(version[pos].data.load(std::memory_order_acquire) != expected_version) {
        }
        uint64_t expected = 0; 
        while(!flags[pos].data.compare_exchange_strong(expected, 1)){
            expected = 0;
        }
        buffer[pos].data = val;
        flags[pos].data.store(2, std::memory_order_release);
        version[pos].data.fetch_add(1, std::memory_order_release);
        // std::cout << "Wrote " << val.data << ": " << val.writer << std::endl;
        return true;
    }
    
    /*
        Reader always reads sequentially. We rely on producers being sane to make consumption
        only depend on the slot being full.
    */
    std::optional<T> try_read() {
        uint64_t read_pos = tail.data.load(std::memory_order_acquire);
        read_pos = read_pos % num_elements;
        if (!(flags[read_pos].data.load() == 2)) { return {}; }
        auto ret = buffer[read_pos];
        flags[read_pos].data.store(0);
        tail.data.store(tail.data.load() + 1, std::memory_order_release);
        return ret.data;
    }
};
