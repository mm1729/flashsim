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
    std::thread* t = 0;
    Worker* w = 0;
    printf("if null: %s\n", w);
    for(int i = 0; i < workers; i++) {
        w = new Worker();
        printf("w: %s ", w);
        all_workers->push_back(w);
        printf("pushed worker\n");
        t = new std::thread(&Worker::stop, w);
        t->join();
        printf("why???\n");
        t = new std::thread(&Worker::run, w);
        printf("t: %s ", t);
        printf("initalized thread\n");
        threads->push_back(t);
        printf("pushed thread\n");
        w = 0;
        t = 0;
    }
    printf("initalized everything\n");
    running = true;
    main_thread = new std::thread(&Dispatcher::consume, this);
}

void Dispatcher::consume() {
    while(running) {
        Event* event = NULL ;
		{
			printf("here2\n");
			std::unique_lock<std::mutex> lck(controller.mtx);
			while (controller.num_requests == 0) {
				//wait++;
				controller.cv.wait(lck);
			}
			//printf("waited for %d\n", wait);
			//wait = 0;
			event = &controller.requests.front();
			controller.requests.pop();
        }

        if(event != NULL) {
            this->addRequest(event);
        }
			

        printf(" after requests added \n num controller requests %d\n", controller.num_requests);

        controller.num_requests--;

        // remove later
        //if(controller.num_requests != 0) controller.num_requests--;

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

void Dispatcher::addRequest(Event* event) {
    Request* rq = new Request(event, ftl);

    // Get the logical package of the address
    Address* address = new Address();
    address->set_linear_address(event->get_logical_address());
    uint package = address->package;
    delete address; // don't need this anymore

    workers_mutex.lock();
    // put the request in correct worker request queue
    Worker* worker = (*all_workers)[package];
    std::mutex* requests_mtx;
    worker->getMutex(requests_mtx);
    requests_mtx->lock();
    worker->addRequest(rq);

    std::condition_variable* cv;
    worker->getCondition(cv);
    cv->notify_one(); // notify thread a request is added

    requests_mtx->unlock();
    workers_mutex.unlock();        
}
