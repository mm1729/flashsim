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

#define SIZE 65536

using namespace ssd;

int main()
{
	load_config();
	print_config(NULL);
   //printf("Press ENTER to continue...");
   //getchar();
   printf("\n");

	Ssd *ssd = new Ssd();

	double result;

//	// Test one write to some blocks.
//	for (int i = 0; i < SIZE; i++)
//	{
//		/* event_arrive(event_type, logical_address, size, start_time) */
//		result = ssd -> event_arrive(WRITE, i*100000, 1, (double) 1+(250*i));
//
//		printf("Write time: %.20lf\n", result);
////		result = ssd -> event_arrive(WRITE, i+10240, 1, (double) 1);
////
//	}
//	for (int i = 0; i < SIZE; i++)
//	{
//		/* event_arrive(event_type, logical_address, size, start_time) */
//		result = ssd -> event_arrive(READ, i*100000, 1, (double) 1+(500*i));
//		printf("Read time : %.20lf\n", result);
////		result = ssd -> event_arrive(READ, i, 1, (double) 1);
////		printf("Read time : %.20lf\n", result);
//	}

//	// Test writes and read to same block.
//	for (int i = 0; i < SIZE; i++)
//	{
//		result = ssd -> event_arrive(WRITE, i%64, 1, (double) 1+(250*i));
//
//		printf("Write time: %.20lf\n", result);
//	}
//	for (int i = 0; i < SIZE; i++)
//	{
//		result = ssd -> event_arrive(READ, i%64, 1, (double) 1+(500*i));
//		printf("Read time : %.20lf\n", result);
//	}

	// Test random writes to a block
	/*result = ssd -> event_arrive(WRITE, 5, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
	result = ssd -> event_arrive(WRITE, 4, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
	result = ssd -> event_arrive(WRITE, 3, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
	result = ssd -> event_arrive(WRITE, 2, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
	result = ssd -> event_arrive(WRITE, 1, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
	result = ssd -> event_arrive(WRITE, 0, 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);*/


	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, SIZE);
	int size;
	for (size = 2; size < 65536; size*=2) {
		for (int i = 0; i < size; i++)
		{
			/* event_arrive(event_type, logical_address, size, start_time) */
			//result = ssd -> event_arrive(WRITE, 6+i, 1, (double) 1800+(300*i));
			//result = ssd->event_arrive(READ, i, 1, (double) 0.0);
			result = ssd->event_arrive(WRITE, distribution(generator), 1, (double) i);
			//printf("Write time: %.20lf\n", result);
		}
		printf("%.20lf\n", ssd->get_time_taken());
		ssd->reset_time_taken();
	}

	printf("%.20lf\n", ssd->get_time_taken());
	// Force Merge
	result = ssd -> event_arrive(WRITE, 10 , 1, (double) 0.0);
	printf("Write time: %.20lf\n", result);
//	for (int i = 0; i < SIZE; i++)
//	{
//		/* event_arrive(event_type, logical_address, size, start_time) */
//		result = ssd -> event_arrive(READ, i%64, 1, (double) 1+(500*i));
//		printf("Read time : %.20lf\n", result);
//	}

	delete ssd;
	return 0;
}
