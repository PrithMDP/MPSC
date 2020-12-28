#include <iostream>
#include <thread>
#include "mpmc.hpp"
#include <vector>
#include <mutex>
#include <algorithm>

using std::cout;
using std::endl;
using std::mutex;
using std::thread;
using std::vector;

MPMCQ<int> data;
std::atomic<int> total_count;

const int writes_per_writer = num_elements;


void read(int id, vector<int>& vec) {
    static thread_local int count;
    static thread_local int pos = 0;
    thread_local int last_val_count;
    thread_local int last_val;
    
    while(1) {
        auto ret = data.u_read(id);
        if(ret.has_value()) {
            vec.push_back(*ret);
            ++total_count;
        }
        mtx.lock();
        cout << "thread " << id << "has read " << count << endl;
        mtx.unlock();
        if(total_count == 2 * num_elements ) break;
    }
    cout << "reader done " << endl;
    cout << "total count is " << total_count << endl;
}

void write(int id) {
    thread_local int count = 0;
    while(count < writes_per_writer) {
        auto val = data.u_write(id);
        ++count;
    }
    std::cout << "done writer: " << id << endl;
}

bool is_strictly_increasing(const vector<int>& vec) {
    for(int i = 0; i < vec.size() - 1; ++i) {
        if(vec[i] >= vec[i+1]) return false;
    }
    return true;
}

int main() {
    total_count.store(0);
    thread t1(write,0);
    thread t2(write,1);
    
    vector<int> v1;
    vector<int> v2;
    vector<int> v4;
    vector<int> v3;
    v1.reserve(num_elements);
    v2.reserve(num_elements);
    v3.reserve(num_elements);
    v4.reserve(num_elements);
    thread t3(read,1,std::ref(v1));
    thread t4(read,2,std::ref(v2));
    //thread t5(read,2,std::ref(v3));
    //thread t6(read,2,std::ref(v4));

    t1.join();
    t2.join();

    t3.join();
    t4.join();
    //t5.join();
    //t6.join();

    /*
        One test is to make sure that each reader consumes in a strictly increasing order
        since writers are writing unique strictly increasing elements to the queue.
    */
    vector<int> all_reads;
    for(auto x : v1) all_reads.push_back(x);
    for(auto x : v2) all_reads.push_back(x);
    for(auto x : v3) all_reads.push_back(x);
    
    cout << "TOTAL COUNT " << total_count << endl;
 
    std::sort(all_reads.begin(), all_reads.end());
    // should contain all numbers from 1 ... (num threads) * (insertions per thread)

}
