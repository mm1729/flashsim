/* Copyright 2009, 2010 Brendan Tauras */

/* ssd_controller.cpp is part of FlashSim. */

/* FlashSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version. */

/* FlashSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License
 * along with FlashSim.  If not, see <http://www.gnu.org/licenses/>. */

/****************************************************************************/

/* Controller class
 *
 * Brendan Tauras 2009-11-03
 *
 * The controller accepts read/write requests through its event_arrive method
 * and consults the FTL regarding what to do by calling the FTL's read/write
 * methods.  The FTL returns an event list for the controller through its issue
 * method that the controller buffers in RAM and sends across the bus.  The
 * controller's issue method passes the events from the FTL to the SSD.
 *
 * The controller also provides an interface for the FTL to collect wear
 * information to perform wear-leveling.
 */

#include <new>
#include <assert.h>
#include <stdio.h>
#include "ssd.h"
#include <atomic>

using namespace ssd;

//----------------------------------
Dispatcher* dispatcher;
std::thread* consumer_thread;
std::vector<std::thread*>* threads;
std::vector<Controller_Worker*>* all_workers;
std::atomic<int> pending_requests;
//----------------------------------

Controller::Controller(Ssd &parent):
	ssd(parent)
{
	// start worker threads

	/*all_workers = new std::vector<Controller_Worker*>();
    threads = new std::vector<std::thread*>();
    std::thread* t = 0;
    Controller_Worker* w = 0;
    //printf("if null: %s\n", w);
    for(int i = 0; i < SSD_SIZE; i++) {
        w = new Controller_Worker(*this);
        //printf("w: %s ", w);
        all_workers->push_back(w);
        //printf("pushed worker\n");
        //t = new std::thread(&Worker::stop, w);
        //t->join();
        //printf("why???\n");
        t = new std::thread(&Controller_Worker::run, w);
        //printf("t: %s ", t);
        //printf("initalized thread\n");
        threads->push_back(t);
        //printf("pushed thread\n");
        t->detach();
        w = 0;
        t = 0;
    }*/

	// start consumer thread
	c_running = true;
	num_c_requests = 0;
	ulock=std::unique_lock<std::mutex>(ulock_mtx);
	consumer_thread = new std::thread(&Controller::consumer, this);


	//-----------------------------
	// Initialize requests done statically
	//------------------------------
	total_time_taken = 0;
	switch (FTL_IMPLEMENTATION)
	{
	case 0:
		ftl = new FtlImpl_Page(*this);
		break;
	case 1:
		ftl = new FtlImpl_Bast(*this);
		break;
	case 2:
		ftl = new FtlImpl_Fast(*this);
		break;
	case 3:
		ftl = new FtlImpl_Dftl(*this);
		break;
	case 4:
		ftl = new FtlImpl_BDftl(*this);
		break;
	}
	num_requests = 0;
	dispatcher = new Dispatcher(SSD_SIZE, *this, ftl);
	printf("created dipatcher\n");
	return;
}

Controller::~Controller(void)
{
	delete ftl;
	return;
}

enum status Controller::event_arrive(Event &event)
{
	//printf("in event_arrive\n");
	//------------------------------
	mtx.lock();
	event.start_time = total_time_taken;
	requests.push(event);
	/*if(event.get_logical_address() != 10 || event.get_event_type() != WRITE) {
		fprintf(stderr, "In func: %s: address %ud \n", __func__, event.get_logical_address());
	}*/
	num_requests++;
	cv.notify_one();
	mtx.unlock();
	return SUCCESS;
	//------------------------------
	/*if(event.get_event_type() == READ)
		return ftl->read(event);
	else if(event.get_event_type() == WRITE)
		return ftl->write(event);
	else if(event.get_event_type() == TRIM)
		return ftl->trim(event);
	else
		fprintf(stderr, "Controller: %s: Invalid event type\n", __func__);
	return FAILURE;*/
}

void Controller::add_controller_request(Event e) {
	//printf("added request\n");
	/*if(e.get_logical_address() != 10 || e.get_event_type() != WRITE) {
		fprintf(stderr, "In func: %s: address %ud \n", __func__, e.get_logical_address());
	}*/
	c_mtx.lock();
	c_requests.push(e);
	num_c_requests++;
	pending_requests++;
	c_cv.notify_one();
	c_mtx.unlock();
}

void Controller::consumer() {
	int i = 0;
	while(c_running) {
		Event* event = NULL ;
		Event* temp_event = NULL;
		c_mtx.lock();
		if(num_c_requests > 0 && !c_requests.empty()) {
		
			temp_event = &c_requests.front();
			event = new Event(temp_event->get_event_type(), temp_event->get_logical_address(), temp_event->get_size(), temp_event->get_start_time());
			event->set_address(new Address(temp_event->get_logical_address(), PAGE));
			/*if(event->get_logical_address() != 10 || event->get_event_type() != WRITE) {
				fprintf(stderr, "In func: %s: address %ud \n", __func__, event->get_logical_address());
				printf("is empty?? %d address %ud\n", !c_requests.empty(), c_requests.front().get_logical_address());
			}*/
			if(c_requests.empty()) printf("queue is empty");
			c_requests.pop();
			
			num_c_requests--;

			c_mtx.unlock();

			/*if(event->get_logical_address() != 10 || event->get_event_type() != WRITE) {
				fprintf(stderr, "after In func: %s: address %ud \n", __func__, event->get_logical_address());
				//printf("is empty?? %d address %ud\n", !c_requests.empty(), c_requests.front().get_logical_address());
			}*/
			//process it
			if(event != NULL) {
				//printf("started processing %d %d\n", i, pending_requests.load());
				enum status ret = this->issue(*event);
				pending_requests--;
				//printf("finished processing %d %d \n", i, pending_requests.load());
				i++;
				//printf("issue returned %d\n", ret);
				//this->addRequestToWorker(*event);
			}
			//printf(" num controller consumer requests %d\n", num_c_requests);

		} else {
			c_mtx.unlock();
			//printf("controller consumer going to sleep\n");
			while (num_c_requests == 0) {
				//wait++;
				if(c_cv.wait_for(this->ulock, std::chrono::seconds(1)) == std::cv_status::timeout ) {
						// keep waiting until woken up by producer 
				}
			}
			//printf("controller consumer woken up\n");
		}

	}
}

void Controller::addRequestToWorker(Event &event) {

    // Get the logical package of the address
    //printf("setting address of event %d\n", event==0);
    if(&event.get_address() == NULL) {
        printf("got null address\n");
    }
    uint package = event.get_address().package;


    // put the request in correct worker request queue
    Controller_Worker* worker = (*all_workers)[package];
    std::mutex* requests_mtx;
    worker->getMutex(requests_mtx);
    requests_mtx->lock();
    worker->addRequest(event);
	//pending_requests++;
    std::condition_variable* worker_cv;
    worker->getCondition(worker_cv);
    worker_cv->notify_one(); // notify thread a request is added

    requests_mtx->unlock();      
}

//-----------Controller_Worker------------------------
Controller_Worker::Controller_Worker(Controller &controller) : controller(controller) {
    running = true; 
    ready = false; 
    ulock=std::unique_lock<std::mutex>(mtx);
    this->requests = new std::queue<Event>();
}

void Controller_Worker::run() {
    //printf("in run method\n");
    bool have_requests = 0;
    while(running) {
        //printf("in loop\n");
        this->requests_mutex.lock();
        //printf("locked\n");
        //printf("request queue: %s\n", this->requests->empty());
        have_requests = !this->requests->empty();
        //printf("reached\n");
        //printf("empty queue: %d\n", have_requests);
        if(have_requests) { // have requests process them
            Event rq = this->requests->front();
            this->requests->pop();
            this->ready = this->requests->empty(); // after processing this request the thread has no more requests pending currently
            this->requests_mutex.unlock(); // unlock the requests queue so dispatcher can add requests while we process request
			enum status ret = this->controller.issue(rq);
			if(ret == FAILURE) {
				pending_requests--;
			}
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

void Controller_Worker::getCondition(std::condition_variable* &cv) {
    cv = &(this)->cv;
}

void Controller_Worker::getMutex(std::mutex* &mtx) {
    mtx = &(this)->requests_mutex;
}

void Controller_Worker::addRequest(Event &event) {
    //printf("in add Request\n");
    this->requests->push(event);
    //printf("added request\n"); 
    ready = true; 
}


//----------------------------------------------------


enum status Controller::issue(Event &event_list)
{
	Event *cur;
	//printf("in issue\n");
	/* go through event list and issue each to the hardware
	 * stop processing events and return failure status if any event in the 
	 *    list fails */
	for(cur = &event_list; cur != NULL; cur = cur -> get_next()){
		cur->start_time = total_time_taken;
		if(cur -> get_size() != 1){
			fprintf(stderr, "Controller: %s: Received non-single-page-sized event from FTL. %\n", __func__);
			return FAILURE;
		}
		else if(cur -> get_event_type() == READ)
		{
			if(cur -> get_address().valid <= NONE) {
				printf("error at second assert\n");
			}
			assert(cur -> get_address().valid > NONE);
			if(ssd.bus.lock(cur -> get_address().package, cur -> get_start_time(), BUS_CTRL_DELAY, *cur) == FAILURE
				|| ssd.read(*cur) == FAILURE
				|| ssd.bus.lock(cur -> get_address().package, cur -> get_start_time()+cur -> get_time_taken(), BUS_CTRL_DELAY + BUS_DATA_DELAY, *cur) == FAILURE
				|| ssd.ram.write(*cur) == FAILURE
				|| ssd.ram.read(*cur) == FAILURE
				|| ssd.replace(*cur) == FAILURE)
				return FAILURE;
		}
		else if(cur -> get_event_type() == WRITE)
		{
			assert(cur -> get_address().valid > NONE);
			if(ssd.bus.lock(cur -> get_address().package, cur -> get_start_time(), BUS_CTRL_DELAY + BUS_DATA_DELAY, *cur) == FAILURE
				|| ssd.ram.write(*cur) == FAILURE
				|| ssd.ram.read(*cur) == FAILURE
				|| ssd.write(*cur) == FAILURE
				|| ssd.replace(*cur) == FAILURE)
				return FAILURE;
		}
		else if(cur -> get_event_type() == ERASE)
		{
			assert(cur -> get_address().valid > NONE);
			if(ssd.bus.lock(cur -> get_address().package, cur -> get_start_time(), BUS_CTRL_DELAY, *cur) == FAILURE
				|| ssd.erase(*cur) == FAILURE)
				return FAILURE;
		}
		else if(cur -> get_event_type() == MERGE)
		{
			assert(cur -> get_address().valid > NONE);
			assert(cur -> get_merge_address().valid > NONE);
			if(ssd.bus.lock(cur -> get_address().package, cur -> get_start_time(), BUS_CTRL_DELAY, *cur) == FAILURE
				|| ssd.merge(*cur) == FAILURE)
				return FAILURE;
		}
		else if(cur -> get_event_type() == TRIM)
		{
			//pending_requests--;
			return SUCCESS;
		}
		else
		{
			//pending_requests--;
			fprintf(stderr, "Controller: %s: Invalid event type\n", __func__);
			return FAILURE;
		}
		double diff = (cur->start_time + cur->get_time_taken()) - total_time_taken;
		//printf("time now %.20lf time taken %.20lf diff %.20lf\n", total_time_taken, (cur->start_time + cur->get_time_taken()), diff);
		if(diff > 0) {
			total_time_taken += diff;
			//printf("time now %.20lf\n", (cur->start_time + cur->get_time_taken()));
		}

		//pending_requests--;
	}

	// finished processing event delete it
	/*Event *temp = cur;
	while(cur != NULL) {
		temp = cur;
		cur = cur->get_next();
		delete temp;
	}*/
	return SUCCESS;
}

double Controller::get_time_taken() {
	int waited = 0;
	int temp = 0;
	while(pending_requests > 0) {
		//sleep(1);
		waited++;
		
		if(waited==   100000) {
			temp = pending_requests.load();
			//printf("pending requests %d", temp);
			waited = 0;
		}
	}
	return total_time_taken;
}

void Controller::reset_time_taken() {
	total_time_taken = 0;
}

void Controller::translate_address(Address &address)
{
	if (PARALLELISM_MODE != 1)
		return;
}

ssd::ulong Controller::get_erases_remaining(const Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_erases_remaining(address);
}

void Controller::get_least_worn(Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_least_worn(address);
}

double Controller::get_last_erase_time(const Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_last_erase_time(address);
}

enum page_state Controller::get_state(const Address &address) const
{
	assert(address.valid > NONE);
	return (ssd.get_state(address));
}

enum block_state Controller::get_block_state(const Address &address) const
{
	assert(address.valid > NONE);
	return (ssd.get_block_state(address));
}

void Controller::get_free_page(Address &address) const
{
	assert(address.valid > NONE);
	ssd.get_free_page(address);
	return;
}

ssd::uint Controller::get_num_free(const Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_num_free(address);
}

ssd::uint Controller::get_num_valid(const Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_num_valid(address);
}

ssd::uint Controller::get_num_invalid(const Address &address) const
{
	assert(address.valid > NONE);
	return ssd.get_num_invalid(address);
}

Block *Controller::get_block_pointer(const Address & address)
{
	return ssd.get_block_pointer(address);
}

const FtlParent &Controller::get_ftl(void) const
{
	return (*ftl);
}

void Controller::print_ftl_statistics()
{
	ftl->print_ftl_statistics();
}
