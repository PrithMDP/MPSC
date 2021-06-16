/ This file is a "Hello, world!" in C++ language by Clang for wandbox.
#include <iostream>
#include <cstdlib>
#include <vector>
#include <set>
#include <atomic>
#include <thread>
#include <cassert>
#include <sched.h>
#include "mpsc.hpp"

#define _GNU_SOURCE

using std::thread;
using std::cout;
using std::end;;

uint8_t READY   = 0;
uint8_t READING = 1;
uint8_t WRITING = 2;

std::atomic<bool> done {false};

struct ProductData {
    bool dirty {false};
    uint64_t version;
    
    std::atomic<uint8_t> state;
};

struct Symbol {
    ProductData data[2];
};

Symbol D;

MPSCQ<ProductData*> Q;

std::vector<uint64_t> vec;

void read() {
    while(!done.load()) {
        auto rec = Q.try_read();
        if(rec) {
            uint8_t readyVal = READY;
            auto& data = *rec;
            while(!data->state.compare_exchange_strong(readyVal, READING)) {
                readyVal = READY;
            }   
            vec.push_back(data->version);
            data->dirty = false;
            data->state.store(READY);
        }   
    }   
}
void write(Symbol& D) {
    bool written = false;
    int try_idx = 0;

    static uint64_t write_version = 1;
    while(!written) {
        uint8_t readyVal = READY;

        if(D.data[try_idx].state.compare_exchange_strong(readyVal, WRITING)) {
            break;
        }
        ++try_idx;
        readyVal = READY;

        if(D.data[try_idx].state.compare_exchange_strong(readyVal, WRITING)) {
            break;
        }
        try_idx %= 2;
    }
    D.data[try_idx].version = write_version;
    if(!D.data[try_idx].dirty) {
        D.data[try_idx].dirty = true;
        Q.try_write(&(D.data[try_idx]));
     }
     D.data[try_idx].state.store(READY);
     ++write_version;
}

 void write_wrapper()
 {
     int writes = 0;
     while(writes < 1'000'000) {
         write(D);
         ++writes;
     }
     done.store(true);
 }

int main()
{
    std::vector<std::thread> threads;
    for(int i = 0; i< 2; ++i) {
        if(i == 0) {
            threads.push_back(std::thread(read));
        }
        else {
            threads.push_back(std::thread(write_wrapper));
        }
        /* wont work on MAC!
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                    sizeof(cpuset), &cpuset);
        if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
        */
    }

    for(auto& x: threads) {
        x.join();
    }
    assert(std::is_sorted(std::begin(vec), std::end(vec)));
    std::set<int> s(vec.begin(), vec.end());
    assert((vec.size() == s.size()));
}
