/*-------------------------------------------------------------------------
 * drawElements Utility Library
 * ----------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Periodic timer test.
 *//*--------------------------------------------------------------------*/

#include "deTimerTest.h"

#include "deTimer.h"
#include "deRandom.h"
#include "deThread.h"

#include <stdio.h>

static void timerCallback (void* arg)
{
	volatile int* numCalls = (volatile int*)arg;
	++(*numCalls);
}

void deTimer_selfTest (void)
{
	const int		numIters				= 25;
	const int		minInterval				= 1;
	const int		maxInterval				= 100;
	const int		intervalSleepMultiplier	= 5;
	int				iter;
	deRandom		rnd;
	deTimer*		timer					= DE_NULL;
	volatile int	numCalls				= 0;

	deRandom_init(&rnd, 6789);

	timer = deTimer_create(timerCallback, (void*)&numCalls);
	DE_TEST_ASSERT(timer);

	for (iter = 0; iter < numIters; iter++)
	{
		deBool	isSingle		= deRandom_getFloat(&rnd) < 0.25f;
		int		interval		= minInterval + (int)(deRandom_getUint32(&rnd) % (deUint32)(maxInterval-minInterval+1));
		int		expectedCalls	= isSingle ? 1 : intervalSleepMultiplier;
		deBool	scheduleOk		= DE_FALSE;

		printf("Iter %d / %d: %d ms %s timer\n", iter+1, numIters, interval, (isSingle ? "single" : "interval"));
		numCalls = 0;

		if (isSingle)
			scheduleOk = deTimer_scheduleSingle(timer, interval);
		else
			scheduleOk = deTimer_scheduleInterval(timer, interval);

		DE_TEST_ASSERT(scheduleOk);

		deSleep((deUint32)(interval*intervalSleepMultiplier));
		deTimer_disable(timer);
		deSleep((deUint32)interval);

		printf("  timer fired %d times, expected %d\n", numCalls, expectedCalls);
		DE_TEST_ASSERT(!isSingle || numCalls == 1);
	}

	deTimer_destroy(timer);
}
