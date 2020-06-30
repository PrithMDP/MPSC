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


void read(int id, vector<int>& vec) {
    static thread_local int count;
    static thread_local int pos = 0;
    while(1) {
        if(int val = data.read()) {
            vec.push_back(val);
            //cout << "thread " << id << " read idx: " << val << endl;
            ++count;
        }
        else {
            break;
        }
    }
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
    vector<int> v1;
    v1.reserve(num_elements);
    vector<int> v2;
    v2.reserve(num_elements);
    thread t3(read,1,std::ref(v1));
    thread t4(read,2,std::ref(v2));
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
    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());
    
    std::vector<int> v_intersection;
 
    std::set_intersection(v1.begin(), v1.end(),
                          v2.begin(), v2.end(),
                          std::back_inserter(v_intersection));
    assert(v_intersection.size() == 0);
    cout << " Reader 1 indices: " << endl;
    for(auto &x : v1) cout << x << " ";
    cout << endl;
    cout << " Reader 2 indices: " << endl;
    for(auto& x: v2) cout << x << " ";
    cout << endl;

    assert((total_count == num_elements));

}
