#include "ssd.h"
#include <stdio.h>
#include <queue>


using namespace ssd;


Dispatcher::Dispatcher(const ssd::uint workers, Controller &controller, FtlParent* ftl): controller(controller) {
    /*Dispatcher::workers = std::queue<AbstractRequest *>();
    std::mutex Dispatcher::requestsMutex;
    std::mutex Dispatcher::workersMutex;
    std::vector<Worker*> Dispatcher::allWorkers;
    std::vector<std::thread*> Dispatcher::threads;*/
    printf("in dispatcher\n");
    this->ftl = ftl;
    this->all_workers = new std::vector<Worker*>();
    this->threads = new std::vector<std::thread*>();
    ulock=std::unique_lock<std::mutex>(ulock_mtx);
    std::thread* t = 0;
    Worker* w = 0;
    //printf("if null: %s\n", w);
    for(int i = 0; i < workers; i++) {
        w = new Worker();
        //printf("w: %s ", w);
        all_workers->push_back(w);
        //printf("pushed worker\n");
        //t = new std::thread(&Worker::stop, w);
        //t->join();
        //printf("why???\n");
        t = new std::thread(&Worker::run, w);
        //printf("t: %s ", t);
        //printf("initalized thread\n");
        threads->push_back(t);
        //printf("pushed thread\n");
        t->detach();
        w = 0;
        t = 0;
    }
    printf("initalized everything\n");
    running = true;
    main_thread = new std::thread(&Dispatcher::consume, this);
}

void Dispatcher::consume() {
    while(running) {
 
        controller.mtx.lock();
        if(controller.num_requests > 0 && !controller.requests.empty()) {
        
            Event event = controller.requests.front();
            controller.requests.pop();
            controller.num_requests--;
            controller.mtx.unlock();

            /*if(event.get_logical_address() != 10 || event.get_event_type() != WRITE) {
		        fprintf(stderr, "In func: %s: address %ud \n", __func__, event.get_logical_address());
	        }*/
            
            //process request
            if(&event != NULL) {
                //controller.add_controller_request(event);
                this->addRequest(event);
            }
            //printf(" num dispatcher consumer requests %d\n", controller.num_requests);




        } else {
            controller.mtx.unlock();

            while (controller.num_requests == 0) {
                //wait++;
                if(controller.cv.wait_for(this->ulock, std::chrono::seconds(1)) == std::cv_status::timeout ) {
                        // keep waiting until woken up by producer 
                        //break;
                }
            }

        }

	}
}

bool Dispatcher::stop() {
    for(int i = 0; i < all_workers->size(); i++) {
        (*all_workers)[i]->stop();
    }

    for(int j = 0; j < threads->size(); j++) {
        (*threads)[j]->join();
    }
}

void Dispatcher::addRequest(Event &event) {
    Request* rq = new Request(event, ftl);

    // Get the logical package of the address
    //printf("setting address of event %d\n", event==0);
    if(&event.get_address() == NULL) {
        printf("got null address\n");
    }

    /*if(event.get_logical_address() != 10 || event.get_event_type() != WRITE) {
		fprintf(stderr, "In func: %s: address %ud \n", __func__, event.get_logical_address());
	}*/
    uint package = event.get_address().package;


    //printf("calling worker %d\n", package);
    workers_mutex.lock();
    // put the request in correct worker request queue
    Worker* worker = (*all_workers)[package];
    std::mutex* requests_mtx;
    worker->getMutex(requests_mtx);
    requests_mtx->lock();
    worker->addRequest(rq);

    std::condition_variable* worker_cv;
    worker->getCondition(worker_cv);
    worker_cv->notify_one(); // notify thread a request is added

    requests_mtx->unlock();
    workers_mutex.unlock();        
}
