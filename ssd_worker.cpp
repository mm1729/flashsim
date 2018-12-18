#include "ssd.h"
#include <stdio.h>
#include <chrono>
#include <queue>

using namespace ssd;

Worker::Worker() {
    running = true; 
    ready = false; 
    ulock=std::unique_lock<std::mutex>(mtx);
    this->requests = new std::queue<Request*>();
}

void Worker::run() {
    //printf("in run method\n");
    bool have_requests = 0;
    Request* rq;
    while(running) {
        //printf("in loop\n");
        this->requests_mutex.lock();
        //printf("locked\n");
        //printf("request queue: %s\n", this->requests->empty());
        have_requests = !this->requests->empty();
        //printf("reached\n");
        //printf("empty queue: %d\n", have_requests);
        if(have_requests) { // have requests process them
            rq = this->requests->front();
            this->requests->pop();
            this->ready = this->requests->empty(); // after processing this request the thread has no more requests pending currently
            this->requests_mutex.unlock(); // unlock the requests queue so dispatcher can add requests while we process request
            /*if(rq->getValue().get_logical_address() != 10) {
		        fprintf(stderr, "In func: %s: address %ud \n", __func__, rq->getValue().get_logical_address());
	        }*/
            rq->process();
        } else {  // no requests go to sleep
            this->ready = false;
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

void Worker::addRequest(Request* event) {
    //printf("in add Request\n");
    this->requests->push(event);
    //printf("added request\n"); 
    ready = true; 
}