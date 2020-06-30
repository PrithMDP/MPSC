#include <iostream>
#include <thread>
#include "mpmc.hpp"
#include <vector>
#include <mutex>

using std::cout;
using std::endl;
using std::mutex;
using std::thread;

MPMCQ<int> data;
std::atomic<int> total_count;

void read(int id) {
    static thread_local int count;
    static thread_local int pos = 0;
    while(1) {
        if(int val = data.read()) {
            //cout << "thread " << id << " read idx: " << val << endl;
            ++count;
        }
        else {
            break;
        }
    }
    std::cout << "TOTAL READ IS " << count << std::endl;
    total_count += count;
}

void write() {
    while(data.write()) {}
}

int main() {
    total_count.store(0);
    thread t1(write);
    //thread t2(write);
    
    for(int i = 0; i < 1000000; ++i) {
        //total_count += 1;
        //total_count -= 1;
    }
    
    thread t3(read,1);
    thread t4(read,2);
    //thread t5(read);
    //thread t6(read);

    t1.join();
    //t2.join();

    t3.join();
    t4.join();
    //t5.join();
    //t6.join();

    int count = 0;
    for(int i = 0; i < num_elements; ++i) {
        count = data.buffer[i].data == 100 ? count + 1 : count;
    }
    if(count != num_elements) {
        cout << "DID NOT WRITE ALL " << num_elements << " elements!";
    }
    cout << "total read element count is " << total_count << endl;

}
