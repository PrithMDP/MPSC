#include <iostream>
#include <atomic>
#include <unordered_map>
#include "../MPSC/mpsc.hpp"

const int MAXSTOCKS = 10'000;

struct Data {
    const int idx;
    int product_id;
    double bid;
    double ask;
};

struct Stock {
    Data first {1,0,0,0}; 
    Data second {2,0,0,0};

};

MPSCQ<Data*> Buffer;

struct Cache {
    Stock arr[MAXSTOCKS] stocks;
    std::atomic<int> read_idxs  [MAXSTOCKS];
    std::atomic<int> write_idxs [MAXSTOCKS];

    void read(Data* ptr) {
        int id = ptr->product_id;
        read_idxs[id].store(ptr->idx);
        while(write_idxs[id].load() == read_idxs[id].load()) {}
        // get data
        read_idxs[id].store(0);
    }   
        
    void write(const Data& d) {
        int id = ptr->product_id;
        write_idxs[id].store(1);
        if(read_idxs[id].load() == 1) write_idxs[id].store(2);
        // write 2
        write_idxs[id].store(0);
    }   
};
