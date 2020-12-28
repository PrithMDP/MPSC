#include <iostream>
#include <thread>
#include "mpsc.hpp"
#include "queue.hpp"
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

MPSCQ<item> data;
//NaiveQ<item> data;
std::atomic<bool> keep_writing = true;
std::atomic<bool> keep_reading = true;

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
    thread t1(write,0);
    thread t2(write,1);

    vector<item> reads;
    reads.reserve(num_elements);
    thread t3(read,1,std::ref(reads));

    t1.join();
    t2.join();
    t3.join();
    
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
            /*
            for(auto x: item.second) {
                cout << x << endl;
            }
            for(int i = 0; i < item.second.size() - 1; ++i) {
                if(item.second[i] >= item.second[i+1]) {
                    cout << item.second[i] << " " << item.second[i+1] << endl;
                }
            }*/
        }
        else {
            cout << "VECTOR IS SORTED" << endl;
        }
    }
    
}
