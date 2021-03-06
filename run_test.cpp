/* Copyright 2009, 2010 Brendan Tauras */

/* run_test.cpp is part of FlashSim. */

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

/* Basic test driver
 * Brendan Tauras 2009-11-02
 *
 * driver to create and run a very basic test of writes then reads */

#include "ssd.h"
#include <random>


#include <sys/time.h>
#include <ctime>



#define SIZE 1048576

using namespace ssd;

typedef long long int64; typedef unsigned long long uint64;
uint64 GetTimeMs64()
{
#ifdef _WIN32
 /* Windows */
 FILETIME ft;
 LARGE_INTEGER li;

 /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
  * to a LARGE_INTEGER structure. */
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;

 uint64 ret = li.QuadPart;
 ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
 ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

 return ret;
#else
 /* Linux */
 struct timeval tv;

 gettimeofday(&tv, NULL);

 uint64 ret = tv.tv_usec;
 /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
 ret /= 1000;

 /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
 ret += (tv.tv_sec * 1000);

 return ret;
#endif
}

int main()
{
	load_config();
	print_config(NULL);

	Ssd *ssd = new Ssd();

	double result;


	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1, 1048576);
	int size;

	int reach_ftl = 0;
	int in_ftl = 0;
	int in_controller = 0;
	
	// Change num_events for different number of events
	int num_events = 1000;

	uint64 before = GetTimeMs64();

	for (int i = 0; i < num_events; i++)
	{
		result += ssd->event_arrive(READ, i, 1, (double) 0.0); // seq read
		//result += ssd->event_arrive(READ, distribution(generator), 1, (double) 0.0); // random read
		//result += ssd->event_arrive(WRITE, distribution(generator), 1, (double) 0.0); // random write
		//result += ssd->event_arrive(WRITE, i, 1, (double) 0.0); // seq write
	}



	//printf("sent all requests\n");
	//printf("Sim time (in microseconds) %.20lf\n", ssd->get_time_taken());
	ssd->get_timing(&reach_ftl, &in_ftl, &in_controller);
	uint64 after = GetTimeMs64();
	uint64 ftlTime = in_controller - (after - before)*1000000;
	ftlTime /= num_events;
	in_controller /= num_events;
	printf("%llu %d\n", ftlTime, in_controller);
	ssd->reset_time_taken();

	delete ssd;
	return 0;
}
