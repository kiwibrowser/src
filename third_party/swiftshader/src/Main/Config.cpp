// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Config.hpp"

#include "Common/Thread.hpp"
#include "Common/Timer.hpp"

namespace sw
{
	Profiler profiler;

	Profiler::Profiler()
	{
		reset();
	}

	void Profiler::reset()
	{
		framesSec = 0;
		framesTotal = 0;
		FPS = 0;

		#if PERF_PROFILE
			for(int i = 0; i < PERF_TIMERS; i++)
			{
				cycles[i] = 0;
			}

			ropOperations = 0;
			ropOperationsTotal = 0;
			ropOperationsFrame = 0;

			texOperations = 0;
			texOperationsTotal = 0;
			texOperationsFrame = 0;

			compressedTex = 0;
			compressedTexTotal = 0;
			compressedTexFrame = 0;
		#endif
	};

	void Profiler::nextFrame()
	{
		#if PERF_PROFILE
			ropOperationsFrame = sw::atomicExchange(&ropOperations, 0);
			texOperationsFrame = sw::atomicExchange(&texOperations, 0);
			compressedTexFrame = sw::atomicExchange(&compressedTex, 0);

			ropOperationsTotal += ropOperationsFrame;
			texOperationsTotal += texOperationsFrame;
			compressedTexTotal += compressedTexFrame;
		#endif

		static double fpsTime = sw::Timer::seconds();

		double time = sw::Timer::seconds();
		double delta = time - fpsTime;
		framesSec++;

		if(delta > 1.0)
		{
			FPS = framesSec / delta;

			fpsTime = time;
			framesTotal += framesSec;
			framesSec = 0;
		}
	}
}