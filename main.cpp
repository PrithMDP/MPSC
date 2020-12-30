#include <iostream>
#include <thread>
#include "mpsc.hpp"
#include "queue.hpp"
#include "linkedlist.hpp"
#include "vectorqueue.hpp"
// #include "finegrained.hpp"
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <chrono>

using std::cout;
using std::endl;
using std::mutex;
using std::thread;
using std::vector;
using std::unordered_map;

struct item {
    uint64_t data;
    int writer;
};


#ifdef LINKED
LLQ<item> data;
#elif LOCKED 
NaiveQ<item> data;
#elif VECLOCKED
VecQ<item> data;
#else
MPSCQ<item> data;
#endif

const int max_writes = 50'000'000;
std::atomic<uint64_t> total_writes;

std::mutex cout_mtx;


void read(int id) {
    std::unordered_map<uint64_t,uint64_t> mapping;
    uint64_t total_reads = 0;
    // while(total_reads != max_writes) {
    while(true) {
        auto val  = data.try_read();
        if(!val){
            continue;
        }
        if(mapping.find(val->writer) != mapping.end()) {
            if(mapping[val->writer] != val->data - 1) {
                std::cout << "ERROR for writer " << val->writer << " .Current is " << mapping[val->writer] <<" .Next is: " << val->data << ".from writer " << val->writer << endl ;
                std::cout << "Reads are " << total_reads << std::endl;
                std::terminate();
            }
            mapping[val->writer] = val->data;
        }
        else {
            mapping[val->writer] = val->data;
        }
        total_reads++;
        if(total_reads % max_writes == 0) {
            std::cout << "Stats: total reads/writes are: " << total_reads << std::endl;
            for(auto &x: mapping) {
                cout << "Writer: " << x.first << " is at " << x.second << std::endl;
            }
        }
    }
    std::cout << " Done reading " << endl;
}

void write(int id) {
    static thread_local uint64_t val = 0;
    //while(total_writes.load() <= max_writes) { 
    while(true) { 
        if(data.try_write({val,id})) {
            //cout_mtx.lock();
            //cout << id << " writing value : "<< val << endl;
            //cout_mtx.unlock();
            total_writes.fetch_add(1);
            ++val;
        }
    }
    std::cout << id << " Done writing " << endl;
}

int main() {
    thread writer1(write,0);
    thread writer2(write,1);
    thread writer3(write,2);
    thread writer4(write,3);
    thread writer5(write,4);

    thread reader(read,1);

    writer1.join();
    writer2.join();
    writer3.join();
    writer4.join();
    writer5.join();

    reader.join();
    
    std::cout << "Total writes were: " << total_writes << endl;
    return 0;
}
