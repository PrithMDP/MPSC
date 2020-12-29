#include <iostream>
#include <thread>
#include "mpsc.hpp"
#include "queue.hpp"
#include "linkedlist.hpp"
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
    int data;
    int writer;
};

#ifdef LOCKED
//NaiveQ<item> data;
LLQ<item> data;
#else
MPSCQ<item> data;
#endif
std::atomic<bool> keep_writing = true;
std::atomic<bool> keep_reading = true;

//const int max_writes = 50'000'000;
//const int max_writes = 50'000'000;
const int max_writes = 50'000'000;
std::atomic<int> total_writes;

std::mutex cout_mtx;
void read(int id, vector<item>& vec) {
    // using namespace std::chrono_literals;
    int total_reads = 0;
    while(total_reads != max_writes) {
        auto val  = data.try_read();
        if(!val){
            continue;
        }
        vec.push_back(val.value());
        total_reads++;
    }
    std::cout << " Done reading " << endl;
}

void write(int id) {
    static thread_local int val = 0;
    while(total_writes.load() <= max_writes) { 
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

    vector<item> reads;
    reads.reserve(num_elements);
    thread reader(read,1,std::ref(reads));

    writer1.join();
    writer2.join();
    writer3.join();
    writer4.join();
    writer5.join();

    reader.join();
    
    std::cout << "Total writes were: " << total_writes << endl;
    
    unordered_map<int,vector<int>> result;
    for(auto x: reads) {
        result[x.writer].push_back(x.data);
    }
    // data should be consumer in order per writer
    // each writer writes with its own seq num
    
    for(auto item: result) {
        if(!std::is_sorted(item.second.begin(), item.second.end())) {
            std::cout << "VECTOR IS NOT SORTED! ERROR" << std::endl;
            
            for(int i = 0; i < item.second.size() - 1; ++i) {
                if(item.second[i] >= item.second[i+1]) {
                    cout << item.second[i] << " " << item.second[i+1] << endl;
                }
            }
        }
        else {
            cout << "VECTOR IS SORTED: Size " << item.second.size() << endl;
            for(int i = 0; i < item.second.size() - 1; ++i) {                                        
                if(item.second[i] >= item.second[i+1]) {
                    cout << item.second[i] << " " << item.second[i+1] << endl;
                }
            }
        }
    }
    
}
