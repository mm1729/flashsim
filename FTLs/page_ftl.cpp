/* Copyright 2011 Matias Bjørling */

/* page_ftl.cpp  */

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

/* Implements a very simple page-level FTL without merge */

#include <new>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../ssd.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace ssd;

/*std::thread main_thread;
bool running;
Dispatcher* dispatcher;*/

FtlImpl_Page::FtlImpl_Page(Controller &controller):
	FtlParent(controller)
{
	trim_map = new bool[NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE];

	numPagesActive = 0;

	//running = true;
	//main_thread = std::thread(&FtlImpl_Page::consume, this);

	//printf("here in creating thread\n");

	//main_thread.join();

	return;
}

FtlImpl_Page::~FtlImpl_Page(void)
{

	return;
}

/*void stop(void)
{
	running = false;
}*/

/*void FtlImpl_Page::consume(void)
{
	//printf("here\n");
	//int wait = 0;
	while(running) {
		{
			printf("here2\n");
			std::unique_lock<std::mutex> lck(controller.mtx);
			while (controller.num_requests == 0) {
				//wait++;
				controller.cv.wait(lck);
			}
			//printf("waited for %d\n", wait);
			//wait = 0;
			Event event = controller.requests.front();
			controller.requests.pop();


			dispatcher->addRequest(&event);			

			printf(" after requests added \n num controller requests %d\n", controller.num_requests);

			controller.num_requests--;

			// remove later
			//if(controller.num_requests != 0) controller.num_requests--;
		}
	}
}*/

void FtlImpl_Page::read_(Event &event) {
	//event.set_address(Address(0, PAGE));
	/*if(event.get_logical_address() != 10) {
		fprintf(stderr, "In func: %s: address %ud \n", __func__, event.get_logical_address());
	}*/
	event.set_noop(true);

	controller.stats.numFTLRead++;

	//controller.issue(event);
	controller.add_controller_request(event);
}

void FtlImpl_Page::write_(Event &event)
{
	//event.set_address(Address(1, PAGE));
	event.set_noop(true);

	controller.stats.numFTLWrite++;

	if (numPagesActive == NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE)
	{
		numPagesActive -= BLOCK_SIZE;

		Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
		eraseEvent.set_address(Address(0, PAGE));

		controller.add_controller_request(eraseEvent);

		//if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");

		//event.incr_time_taken(eraseEvent.get_time_taken());

		controller.stats.numFTLErase++;
	}

	numPagesActive++;


	controller.add_controller_request(event);
}

enum status FtlImpl_Page::read(Event &event)
{
	event.set_address(Address(0, PAGE));
	event.set_noop(true);

	controller.stats.numFTLRead++;

	return controller.issue(event);
}

enum status FtlImpl_Page::write(Event &event)
{
	event.set_address(Address(1, PAGE));
	event.set_noop(true);

	controller.stats.numFTLWrite++;

	if (numPagesActive == NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE)
	{
		numPagesActive -= BLOCK_SIZE;

		Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
		eraseEvent.set_address(Address(0, PAGE));

		if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");

		event.incr_time_taken(eraseEvent.get_time_taken());

		controller.stats.numFTLErase++;
	}

	numPagesActive++;


	return controller.issue(event);
}

enum status FtlImpl_Page::trim(Event &event)
{
	controller.stats.numFTLTrim++;

	uint dlpn = event.get_logical_address();

	if (!trim_map[event.get_logical_address()])
		trim_map[event.get_logical_address()] = true;

	// Update trim map and update block map if all pages are trimmed. i.e. the state are reseted to optimal.
	long addressStart = dlpn - dlpn % BLOCK_SIZE;
	bool allTrimmed = true;
	for (uint i=addressStart;i<addressStart+BLOCK_SIZE;i++)
	{
		if (!trim_map[i])
			allTrimmed = false;
	}

	if (allTrimmed)
	{
		Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
		eraseEvent.set_address(Address(0, PAGE));

		if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");

		event.incr_time_taken(eraseEvent.get_time_taken());

		for (uint i=addressStart;i<addressStart+BLOCK_SIZE;i++)
			trim_map[i] = false;

		controller.stats.numFTLErase++;

		numPagesActive -= BLOCK_SIZE;
	}

	return SUCCESS;
}
