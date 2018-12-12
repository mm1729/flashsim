#include "ssd.h"
#include <stdio.h>
#include <chrono>

using namespace ssd;

void Worker::run() {
    bool have_requests = 0;
    Request* rq;
    while(running) {
        this->requests_mutex.lock();
        have_requests = this->requests.empty();
        if(have_requests) { // have requests process them
            rq = this->requests.front();
            this->requests.pop();
            this->ready = this->requests.empty(); // after processing this request the thread has no more requests pending currently
            this->requests_mutex.unlock(); // unlock the requests queue so dispatcher can add requests while we process request
            rq->process();
        } else {  // no requests go to sleep
            this->requests_mutex.unlock(); // unlock requests queue and go to sleep
            while(!ready && running) {
                if(this->cv.wait_for(this->ulock, std::chrono::seconds(1)) == std::cv_status::timeout ) {
                    // keep waiting until woken up by dispatcher 
                }
            }
        }
    }
}

void Worker::getCondition(std::condition_variable* &cv) {
    cv = &(this)->cv;
}

void Worker::getMutex(std::mutex* &mtx) {
    mtx = &(this)->requests_mutex;
}